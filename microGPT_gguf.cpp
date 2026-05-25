// microGPT_gguf.cpp
// Based on microGPT.cpp, adds GGUF save/load functionality: train -> save -> load -> predict
//
// Usage:
//   ./microGPT_gguf              -> train + save model to model.gguf + inference
//   ./microGPT_gguf model.gguf   -> load model directly + inference (skip training)
//
// GGUF format (simplified, compatible with llama.cpp design philosophy):
//   [magic: 4B] [version: 4B] [n_tensors: 4B] [n_metadata: 4B]
//   [metadata KV pairs...]
//   [tensor info entries...]
//   [tensor data...]
//
// Compile: g++ -std=c++17 -O2 -o microGPT_gguf microGPT_gguf.cpp

// microGPT_gguf.cpp
// 
// Compile: g++ -std=c++20 -O2 -o microGPT_gguf microGPT_gguf.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <memory>
#include <functional>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <span>

// POSIX headers for mmap
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// ============================================================================
// Part 1: Autograd Engine (needed for training)
// ============================================================================

struct Value;
using ValuePtr = std::shared_ptr<Value>;
using Vec1D = std::vector<ValuePtr>;
using Vec2D = std::vector<Vec1D>;

struct Value : public std::enable_shared_from_this<Value> {
    double data;
    double grad;
    std::vector<ValuePtr> _children;
    std::vector<double> _local_grads;

    Value(double data) : data(data), grad(0.0) {}

    void backward() {
        std::vector<ValuePtr> topo;
        std::set<Value*> visited;

        std::function<void(ValuePtr)> build_topo = [&](ValuePtr v) {
            if (visited.find(v.get()) == visited.end()) {
                visited.insert(v.get());
                for (auto& child : v->_children) build_topo(child);
                topo.push_back(v);
            }
        };

        build_topo(shared_from_this());
        grad = 1.0;

        for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
            auto v = *it;
            for (size_t i = 0; i < v->_children.size(); ++i) {
                v->_children[i]->grad += v->_local_grads[i] * v->grad;
            }
        }
    }
};

ValuePtr make_val(double d) { return std::make_shared<Value>(d); }

ValuePtr operator+(const ValuePtr& a, const ValuePtr& b) {
    auto out = make_val(a->data + b->data);
    out->_children = {a, b}; out->_local_grads = {1.0, 1.0}; return out;
}
ValuePtr operator+(const ValuePtr& a, double b) { return a + make_val(b); }
ValuePtr operator+(double a, const ValuePtr& b) { return make_val(a) + b; }

ValuePtr operator*(const ValuePtr& a, const ValuePtr& b) {
    auto out = make_val(a->data * b->data);
    out->_children = {a, b}; out->_local_grads = {b->data, a->data}; return out;
}
ValuePtr operator*(const ValuePtr& a, double b) { return a * make_val(b); }
ValuePtr operator*(double a, const ValuePtr& b) { return make_val(a) * b; }

ValuePtr operator-(const ValuePtr& a) { return a * (-1.0); }
ValuePtr operator-(const ValuePtr& a, const ValuePtr& b) { return a + (-b); }
ValuePtr operator-(const ValuePtr& a, double b) { return a + (-b); }
ValuePtr operator-(double a, const ValuePtr& b) { return make_val(a) + (-b); }

ValuePtr pow(const ValuePtr& a, double b) {
    auto out = make_val(std::pow(a->data, b));
    out->_children = {a}; out->_local_grads = {b * std::pow(a->data, b - 1)}; return out;
}

ValuePtr operator/(const ValuePtr& a, const ValuePtr& b) { return a * pow(b, -1.0); }
ValuePtr operator/(const ValuePtr& a, double b) { return a * pow(make_val(b), -1.0); }
ValuePtr operator/(double a, const ValuePtr& b) { return make_val(a) * pow(b, -1.0); }

ValuePtr log(const ValuePtr& a) {
    auto out = make_val(std::log(a->data));
    out->_children = {a}; out->_local_grads = {1.0 / a->data}; return out;
}

ValuePtr exp(const ValuePtr& a) {
    auto out = make_val(std::exp(a->data));
    out->_children = {a}; out->_local_grads = {std::exp(a->data)}; return out;
}

ValuePtr relu(const ValuePtr& a) {
    auto out = make_val(std::max(0.0, a->data));
    out->_children = {a}; out->_local_grads = {(a->data > 0) ? 1.0 : 0.0}; return out;
}

// ============================================================================
// Globals & Type Aliases
// ============================================================================

std::mt19937 gen(42);

using DVec = std::vector<double>;     // inference: 1D double vector
using DMat = std::vector<DVec>;       // inference: 2D double matrix

// --- Autograd architecture functions (used for training) ---

Vec2D matrix_val(int nout, int nin, double std_dev = 0.08) {
    std::normal_distribution<double> d(0.0, std_dev);
    Vec2D mat(nout, Vec1D(nin));
    for (int i = 0; i < nout; ++i)
        for (int j = 0; j < nin; ++j)
            mat[i][j] = make_val(d(gen));
    return mat;
}

Vec1D linear_val(const Vec1D& x, const Vec2D& w) {
    Vec1D out(w.size());
    for (size_t i = 0; i < w.size(); ++i) {
        ValuePtr sum = make_val(0.0);
        for (size_t j = 0; j < x.size(); ++j)
            sum = sum + (w[i][j] * x[j]);
        out[i] = sum;
    }
    return out;
}

Vec1D softmax_val(const Vec1D& logits) {
    double max_val = logits[0]->data;
    for (const auto& v : logits) max_val = std::max(max_val, v->data);
    Vec1D exps(logits.size());
    ValuePtr total = make_val(0.0);
    for (size_t i = 0; i < logits.size(); ++i) {
        exps[i] = exp(logits[i] - max_val);
        total = total + exps[i];
    }
    Vec1D out(logits.size());
    for (size_t i = 0; i < exps.size(); ++i) out[i] = exps[i] / total;
    return out;
}

Vec1D rmsnorm_val(const Vec1D& x) {
    ValuePtr ms = make_val(0.0);
    for (const auto& xi : x) ms = ms + (xi * xi);
    ms = ms / static_cast<double>(x.size());
    ValuePtr scale = pow(ms + 1e-5, -0.5);
    Vec1D out(x.size());
    for (size_t i = 0; i < x.size(); ++i) out[i] = x[i] * scale;
    return out;
}

// Pure double linear transform: y = W * x
DVec d_linear(const DVec& x, const DMat& w) {
    DVec out(w.size(), 0.0);
    for (size_t i = 0; i < w.size(); ++i)
        for (size_t j = 0; j < x.size(); ++j)
            out[i] += w[i][j] * x[j];
    return out;
}

// Pure double softmax
DVec d_softmax(const DVec& logits) {
    double max_val = *std::max_element(logits.begin(), logits.end());
    DVec exps(logits.size());
    double total = 0.0;
    for (size_t i = 0; i < logits.size(); ++i) {
        exps[i] = std::exp(logits[i] - max_val);
        total += exps[i];
    }
    for (auto& e : exps) e /= total;
    return exps;
}

// Pure double RMSNorm
DVec d_rmsnorm(const DVec& x) {
    double ms = 0.0;
    for (auto xi : x) ms += xi * xi;
    ms /= (double)x.size();
    double scale = std::pow(ms + 1e-5, -0.5);
    DVec out(x.size());
    for (size_t i = 0; i < x.size(); ++i) out[i] = x[i] * scale;
    return out;
}

// ============================================================================
// Part 3: GGUF format
// ============================================================================

struct GGUFMeta {
    std::string key;
    enum Type { INT = 0, FLOAT = 1, STRING = 2 } type;
    int64_t  int_val;
    double   float_val;
    std::string str_val;
};

struct GGUFTensorInfo {
    std::string name;
    uint32_t nrows;
    uint32_t ncols;
    uint64_t offset;   // byte offset within the file
};

// --- GGUF Writer ---

class GGUFWriter {
    std::ofstream out;
    std::vector<GGUFMeta> metas;
    std::vector<GGUFTensorInfo> tensor_infos;
    std::vector<DMat> tensor_datas;  // buffered; written all at once at the end

public:
    GGUFWriter(const std::string& path) {
        out.open(path, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "ERROR: Cannot open " << path << " for writing\n";
            return;
        }
    }

    void add_meta_int(const std::string& key, int64_t val) {
        metas.push_back({key, GGUFMeta::INT, val, 0.0, ""});
    }
    void add_meta_float(const std::string& key, double val) {
        metas.push_back({key, GGUFMeta::FLOAT, 0, val, ""});
    }
    void add_meta_string(const std::string& key, const std::string& val) {
        metas.push_back({key, GGUFMeta::STRING, 0, 0.0, val});
    }

    void add_tensor(const std::string& name, const DMat& data) {
        uint64_t offset = 0;
        if (!tensor_datas.empty()) {
            auto& last = tensor_infos.back();
            offset = last.offset + (uint64_t)last.nrows * last.ncols * sizeof(float);
        }
        tensor_infos.push_back({name, (uint32_t)data.size(), (uint32_t)data[0].size(), offset});
        tensor_datas.push_back(data);
    }

    void add_tensor_from_val(const std::string& name, const std::vector<std::vector<ValuePtr>>& data) {
        DMat mat(data.size(), DVec(data[0].size()));
        for (size_t i = 0; i < data.size(); ++i)
            for (size_t j = 0; j < data[i].size(); ++j)
                mat[i][j] = data[i][j]->data; 
        add_tensor(name, mat);
    }

    void write() {
        if (!out.is_open()) return;

        // (1) File header
        uint32_t magic = 0x46554747;  // "GGUF" in little-endian
        uint32_t version = 3;
        uint32_t n_tensors = (uint32_t)tensor_infos.size();
        uint32_t n_metas = (uint32_t)metas.size();

        out.write(reinterpret_cast<char*>(&magic), 4);
        out.write(reinterpret_cast<char*>(&version), 4);
        out.write(reinterpret_cast<char*>(&n_tensors), 4);
        out.write(reinterpret_cast<char*>(&n_metas), 4);

        // (2) Metadata
        for (auto& m : metas) {
            uint32_t klen = (uint32_t)m.key.size();
            out.write(reinterpret_cast<char*>(&klen), 4);
            out.write(m.key.data(), klen);

            uint32_t vtype = (uint32_t)m.type;
            out.write(reinterpret_cast<char*>(&vtype), 4);

            if (m.type == GGUFMeta::INT) {
                int64_t v = m.int_val;
                out.write(reinterpret_cast<char*>(&v), 8);
            } else if (m.type == GGUFMeta::FLOAT) {
                double v = m.float_val;
                out.write(reinterpret_cast<char*>(&v), 8);
            } else {
                uint32_t slen = (uint32_t)m.str_val.size();
                out.write(reinterpret_cast<char*>(&slen), 4);
                out.write(m.str_val.data(), slen);
            }
        }

        // (3) Tensor info with 32-Byte Alignment Calculation
        uint64_t current_offset = out.tellp();
        for (auto& t : tensor_infos) {
            current_offset += 4 + t.name.size() + 4 + 4 + 8;
        }

        uint64_t alignment = 32;
        uint64_t padding = (alignment - (current_offset % alignment)) % alignment;
        uint64_t data_start_offset = current_offset + padding;

        for (auto& t : tensor_infos) {
            uint32_t nlen = (uint32_t)t.name.size();
            out.write(reinterpret_cast<char*>(&nlen), 4);
            out.write(t.name.data(), nlen);
            out.write(reinterpret_cast<char*>(&t.nrows), 4);
            out.write(reinterpret_cast<char*>(&t.ncols), 4);
            
            t.offset += data_start_offset; // Apply padding shift
            out.write(reinterpret_cast<char*>(&t.offset), 8);
        }

        // Write alignment padding bytes
        std::vector<char> pad_bytes(padding, 0);
        out.write(pad_bytes.data(), padding);

        // (4) Tensor data (FP32 format)
        for (size_t ti = 0; ti < tensor_datas.size(); ++ti) {
            auto& mat = tensor_datas[ti];
            for (auto& row : mat) {
                for (double val : row) {
                    float fval = (float)val;  
                    out.write(reinterpret_cast<char*>(&fval), sizeof(float));
                }
            }
        }

        out.close();
        std::cout << "[OK] GGUF saved: " << tensor_infos.size() << " tensors, "
                  << metas.size() << " metadata entries (Aligned to " << alignment << " bytes)\n";
    }
};

// --- GGUF Reader (Optimized with mmap) ---

struct GGUFModel {
    int n_layer;
    int n_embd;
    int block_size;
    int n_head;
    int head_dim;
    int vocab_size;
    int BOS;

    std::vector<char> uchars;
    std::map<std::string, DMat> weights;
    std::vector<GGUFMeta> metas;
};

class GGUFReader {
public:
    GGUFModel load(const std::string& path) {
        GGUFModel model;

        // 1. Open file descriptor
        int fd = open(path.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cerr << "ERROR: Cannot open " << path << " for reading\n";
            return model;
        }

        // 2. Get file size
        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            close(fd);
            return model;
        }
        size_t file_size = sb.st_size;

        // 3. Map file into virtual memory
        void* mapped_data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd); // Safe to close fd once mapped

        if (mapped_data == MAP_FAILED) {
            std::cerr << "ERROR: mmap failed\n";
            return model;
        }

        const uint8_t* ptr = static_cast<const uint8_t*>(mapped_data);

        // (1) File header
        uint32_t magic = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
        uint32_t version = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
        uint32_t n_tensors = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
        uint32_t n_metas = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;

        if (magic != 0x46554747) {
            std::cerr << "ERROR: Not a GGUF file (bad magic)\n";
            munmap(mapped_data, file_size);
            return model;
        }
        
        std::cout << "GGUF version: " << version << ", tensors: " << n_tensors
                  << ", metadata: " << n_metas << "\n";

        // (2) Metadata
        for (uint32_t i = 0; i < n_metas; ++i) {
            GGUFMeta m;
            uint32_t klen = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
            m.key = std::string(reinterpret_cast<const char*>(ptr), klen); ptr += klen;

            uint32_t vtype = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
            m.type = (GGUFMeta::Type)vtype;

            if (m.type == GGUFMeta::INT) {
                m.int_val = *reinterpret_cast<const int64_t*>(ptr); ptr += 8;
            } else if (m.type == GGUFMeta::FLOAT) {
                m.float_val = *reinterpret_cast<const double*>(ptr); ptr += 8;
            } else {
                uint32_t slen = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
                m.str_val = std::string(reinterpret_cast<const char*>(ptr), slen); ptr += slen;
            }
            model.metas.push_back(m);
        }

        // Restore hyperparameters
        for (auto& m : model.metas) {
            if (m.key == "n_layer")         model.n_layer    = (int)m.int_val;
            if (m.key == "n_embd")          model.n_embd     = (int)m.int_val;
            if (m.key == "block_size")      model.block_size = (int)m.int_val;
            if (m.key == "n_head")          model.n_head     = (int)m.int_val;
            if (m.key == "head_dim")        model.head_dim   = (int)m.int_val;
            if (m.key == "vocab_size")      model.vocab_size = (int)m.int_val;
            if (m.key == "bos_token")       model.BOS        = (int)m.int_val;
            if (m.key == "tokenizer_chars") {
                model.uchars.assign(m.str_val.begin(), m.str_val.end());
            }
        }
        model.head_dim = model.n_embd / model.n_head;

        // (3) Tensor info
        struct RawTensorInfo { std::string name; uint32_t nrows, ncols; uint64_t offset; };
        std::vector<RawTensorInfo> tinfos(n_tensors);

        for (uint32_t i = 0; i < n_tensors; ++i) {
            uint32_t nlen = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
            tinfos[i].name = std::string(reinterpret_cast<const char*>(ptr), nlen); ptr += nlen;
            tinfos[i].nrows = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
            tinfos[i].ncols = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
            tinfos[i].offset = *reinterpret_cast<const uint64_t*>(ptr); ptr += 8;
        }

        // (4) Read tensor data directly from memory map
        for (auto& ti : tinfos) {
            DMat mat(ti.nrows, DVec(ti.ncols));
            
            // Pointer directly to the aligned tensor data on disk/virtual memory
            const float* tensor_data_ptr = reinterpret_cast<const float*>(
                static_cast<const uint8_t*>(mapped_data) + ti.offset
            );

            size_t idx = 0;
            for (uint32_t r = 0; r < ti.nrows; ++r) {
                for (uint32_t c = 0; c < ti.ncols; ++c) {
                    mat[r][c] = static_cast<double>(tensor_data_ptr[idx++]);
                }
            }
            model.weights[ti.name] = mat;
            std::cout << "  mapped tensor: " << ti.name
                      << " [" << ti.nrows << "x" << ti.ncols << "]\n";
        }

        // Unmap memory after copying to DMat. 
        // (If rewriting inference to use span<float>, this unmap happens in a destructor instead).
        munmap(mapped_data, file_size);

        std::cout << "[OK] GGUF loaded: " << model.weights.size() << " tensors\n";
        return model;
    }
};

// ============================================================================
// Part 4: Pure inference GPT forward pass
// ============================================================================

DVec gpt_inference(
    int token_id, int pos_id,
    const GGUFModel& model,
    std::vector<std::vector<DVec>>& keys,    // KV Cache: keys[layer][timestep]
    std::vector<std::vector<DVec>>& values   // KV Cache: values[layer][timestep]
) {
    // Token + Position Embedding
    DVec tok_emb = model.weights.at("wte")[token_id];
    DVec pos_emb = model.weights.at("wpe")[pos_id];

    DVec x(model.n_embd);
    for (int i = 0; i < model.n_embd; ++i) x[i] = tok_emb[i] + pos_emb[i];

    x = d_rmsnorm(x);

    for (int li = 0; li < model.n_layer; ++li) {
        std::string lpre = "layer" + std::to_string(li) + ".";

        // Multi-Head Attention
        DVec x_residual = x;
        x = d_rmsnorm(x);

        DVec q = d_linear(x, model.weights.at(lpre + "attn_wq"));
        DVec k = d_linear(x, model.weights.at(lpre + "attn_wk"));
        DVec v = d_linear(x, model.weights.at(lpre + "attn_wv"));

        // KV Cache: store current step's k, v
        keys[li].push_back(k);
        values[li].push_back(v);

        DVec x_attn;

        for (int h = 0; h < model.n_head; ++h) {
            int hs = h * model.head_dim;

            DVec q_h(q.begin() + hs, q.begin() + hs + model.head_dim);

            std::vector<DVec> k_h, v_h;
            for (const auto& ki : keys[li]) {
                k_h.push_back(DVec(ki.begin() + hs, ki.begin() + hs + model.head_dim));
            }
            for (const auto& vi : values[li]) {
                v_h.push_back(DVec(vi.begin() + hs, vi.begin() + hs + model.head_dim));
            }

            // Attention scores: Q * K^T / sqrt(head_dim)
            DVec attn_logits;
            for (size_t t = 0; t < k_h.size(); ++t) {
                double sum = 0.0;
                for (int j = 0; j < model.head_dim; ++j)
                    sum += q_h[j] * k_h[t][j];
                attn_logits.push_back(sum / std::sqrt((double)model.head_dim));
            }

            DVec attn_weights = d_softmax(attn_logits);

            // Weighted sum of values
            DVec head_out(model.head_dim, 0.0);
            for (int j = 0; j < model.head_dim; ++j) {
                for (size_t t = 0; t < v_h.size(); ++t)
                    head_out[j] += attn_weights[t] * v_h[t][j];
            }

            x_attn.insert(x_attn.end(), head_out.begin(), head_out.end());
        }

        // Output projection + residual connection
        x = d_linear(x_attn, model.weights.at(lpre + "attn_wo"));
        for (int i = 0; i < model.n_embd; ++i) x[i] += x_residual[i];

        // MLP
        x_residual = x;
        x = d_rmsnorm(x);
        x = d_linear(x, model.weights.at(lpre + "mlp_fc1"));
        for (auto& xi : x) xi = std::max(0.0, xi);  // ReLU
        x = d_linear(x, model.weights.at(lpre + "mlp_fc2"));
        for (int i = 0; i < model.n_embd; ++i) x[i] += x_residual[i];
    }

    return d_linear(x, model.weights.at("lm_head"));
}

// ============================================================================
// Part 5: GGUF save function
// ============================================================================

void save_gguf(
    const std::string& path,
    const std::map<std::string, std::vector<std::vector<ValuePtr>>>& state_dict,
    int n_layer, int n_embd, int block_size, int n_head,
    int vocab_size, int BOS,
    const std::vector<char>& uchars,
    int trained_steps, double final_loss
) {
    GGUFWriter writer(path);

    // (1) Write hyperparameters
    writer.add_meta_string("architecture", "microgpt");
    writer.add_meta_int("n_layer", n_layer);
    writer.add_meta_int("n_embd", n_embd);
    writer.add_meta_int("block_size", block_size);
    writer.add_meta_int("n_head", n_head);
    writer.add_meta_int("head_dim", n_embd / n_head);
    writer.add_meta_int("vocab_size", vocab_size);
    writer.add_meta_int("bos_token", BOS);
    writer.add_meta_int("trained_steps", trained_steps);
    writer.add_meta_float("final_loss", final_loss);

    // (2) Write tokenizer
    std::string tok_str(uchars.begin(), uchars.end());
    writer.add_meta_string("tokenizer_chars", tok_str);

    // (3) Write weights
    for (auto& [name, mat] : state_dict) {
        writer.add_tensor_from_val(name, mat);
    }

    writer.write();
}

// ============================================================================
// Part 6: Sampling / prediction function
// ============================================================================

void predict_samples(const GGUFModel& model, int num_samples = 20, double temperature = 0.5) {
    std::mt19937 gen(42);

    std::cout << "\n-----------------------------------------------\n";
    std::cout << "  Inference from GGUF (temperature=" << temperature << ")\n";
    std::cout << "-----------------------------------------------\n";

    for (int si = 0; si < num_samples; ++si) {
        std::vector<std::vector<DVec>> keys(model.n_layer), values(model.n_layer);
        int token_id = model.BOS;
        std::string sample;

        for (int pos = 0; pos < model.block_size; ++pos) {
            DVec logits = gpt_inference(token_id, pos, model, keys, values);

            // Temperature scaling
            for (auto& l : logits) l /= temperature;

            DVec probs = d_softmax(logits);

            // Sample
            std::discrete_distribution<int> dist(probs.begin(), probs.end());
            token_id = dist(gen);

            if (token_id == model.BOS) break;  // stop on EOS/BOS token
            if (token_id < (int)model.uchars.size())
                sample += model.uchars[token_id];
        }

        printf("  sample %2d: %s\n", si + 1, sample.c_str());
    }
}

// ============================================================================
// Part 7: Main - train -> save, or load -> inference
// ============================================================================

int main(int argc, char* argv[]) {

    std::string gguf_path = "model.gguf";

    // --- Mode 2: load GGUF directly and run inference ---
    if (argc > 1) {
        gguf_path = argv[1];
        std::cout << "[>>] Loading GGUF: " << gguf_path << "\n";

        GGUFReader reader;
        GGUFModel model = reader.load(gguf_path);

        if (model.weights.empty()) {
            std::cerr << "Failed to load model.\n";
            return 1;
        }

        predict_samples(model);
        return 0;
    }

    // --- Mode 1: train -> save GGUF -> load GGUF -> inference ---

    // 1. Dataset
    if (!std::filesystem::exists("input.txt")) {
        std::cout << "Downloading dataset...\n";
        int res = std::system("curl -s -o input.txt https://raw.githubusercontent.com/karpathy/makemore/988aa59/names.txt");
        if (res != 0) { std::cerr << "Download failed.\n"; return 1; }
    }

    std::vector<std::string> docs;
    std::ifstream file("input.txt");
    std::string line;
    while (std::getline(file, line)) {
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (!line.empty()) docs.push_back(line);
    }
    std::mt19937 gen(42);
    std::shuffle(docs.begin(), docs.end(), gen);
    std::cout << "num docs: " << docs.size() << "\n";

    // 2. Tokenizer
    std::set<char> unique_chars;
    for (const auto& d : docs)
        for (char c : d) unique_chars.insert(c);
    std::vector<char> uchars(unique_chars.begin(), unique_chars.end());
    int BOS = uchars.size();
    int vocab_size = uchars.size() + 1;

    // 3. Hyperparameters
    int n_layer = 1, n_embd = 16, block_size = 16, n_head = 4;
    int head_dim = n_embd / n_head;

    // 4. Initialize parameters
    std::map<std::string, std::vector<std::vector<ValuePtr>>> state_dict;
    state_dict["wte"]     = matrix_val(vocab_size, n_embd);
    state_dict["wpe"]     = matrix_val(block_size, n_embd);
    state_dict["lm_head"] = matrix_val(vocab_size, n_embd);

    for (int i = 0; i < n_layer; ++i) {
        std::string p = "layer" + std::to_string(i) + ".";
        state_dict[p + "attn_wq"] = matrix_val(n_embd, n_embd);
        state_dict[p + "attn_wk"] = matrix_val(n_embd, n_embd);
        state_dict[p + "attn_wv"] = matrix_val(n_embd, n_embd);
        state_dict[p + "attn_wo"] = matrix_val(n_embd, n_embd);
        state_dict[p + "mlp_fc1"] = matrix_val(4 * n_embd, n_embd);
        state_dict[p + "mlp_fc2"] = matrix_val(n_embd, 4 * n_embd);
    }

    std::vector<ValuePtr> params;
    for (const auto& [name, mat] : state_dict)
        for (const auto& row : mat)
            for (const auto& p : row)
                params.push_back(p);
    std::cout << "num params: " << params.size() << "\n";

    // 5. GPT forward pass (for training, with autograd)
    auto gpt = [&](int token_id, int pos_id,
                   std::vector<std::vector<Vec1D>>& keys,
                   std::vector<std::vector<Vec1D>>& values) {
        Vec1D tok_emb = state_dict["wte"][token_id];
        Vec1D pos_emb = state_dict["wpe"][pos_id];
        Vec1D x(n_embd);
        for (int i = 0; i < n_embd; ++i) x[i] = tok_emb[i] + pos_emb[i];

        x = rmsnorm_val(x);

        for (int li = 0; li < n_layer; ++li) {
            std::string lpre = "layer" + std::to_string(li) + ".";
            Vec1D x_residual = x;
            x = rmsnorm_val(x);

            Vec1D q = linear_val(x, state_dict[lpre + "attn_wq"]);
            Vec1D k = linear_val(x, state_dict[lpre + "attn_wk"]);
            Vec1D v = linear_val(x, state_dict[lpre + "attn_wv"]);

            keys[li].push_back(k);
            values[li].push_back(v);

            Vec1D x_attn;
            for (int h = 0; h < n_head; ++h) {
                int hs = h * head_dim;
                Vec1D q_h(q.begin() + hs, q.begin() + hs + head_dim);
                std::vector<Vec1D> k_h, v_h;
                for (const auto& ki : keys[li]) k_h.push_back(Vec1D(ki.begin() + hs, ki.begin() + hs + head_dim));
                for (const auto& vi : values[li]) v_h.push_back(Vec1D(vi.begin() + hs, vi.begin() + hs + head_dim));

                Vec1D attn_logits;
                for (size_t t = 0; t < k_h.size(); ++t) {
                    ValuePtr sum = make_val(0.0);
                    for (int j = 0; j < head_dim; ++j) sum = sum + (q_h[j] * k_h[t][j]);
                    attn_logits.push_back(sum / std::sqrt(head_dim));
                }

                Vec1D attn_weights = softmax_val(attn_logits);
                Vec1D head_out(head_dim, make_val(0.0));
                for (int j = 0; j < head_dim; ++j) {
                    ValuePtr sum = make_val(0.0);
                    for (size_t t = 0; t < v_h.size(); ++t) sum = sum + (attn_weights[t] * v_h[t][j]);
                    head_out[j] = sum;
                }
                x_attn.insert(x_attn.end(), head_out.begin(), head_out.end());
            }

            x = linear_val(x_attn, state_dict[lpre + "attn_wo"]);
            for (size_t i = 0; i < x.size(); ++i) x[i] = x[i] + x_residual[i];

            x_residual = x;
            x = rmsnorm_val(x);
            x = linear_val(x, state_dict[lpre + "mlp_fc1"]);
            for (size_t i = 0; i < x.size(); ++i) x[i] = relu(x[i]);
            x = linear_val(x, state_dict[lpre + "mlp_fc2"]);
            for (size_t i = 0; i < x.size(); ++i) x[i] = x[i] + x_residual[i];
        }

        return linear_val(x, state_dict["lm_head"]);
    };

    // 6. Adam optimizer
    double learning_rate = 0.01, beta1 = 0.85, beta2 = 0.99, eps_adam = 1e-8;
    std::vector<double> m(params.size(), 0.0);
    std::vector<double> v(params.size(), 0.0);

    // 7. Training loop
    int num_steps = 1000;
    double final_loss = 0.0;

    std::cout << "\n--- Training (" << num_steps << " steps) ---\n";
    for (int step = 0; step < num_steps; ++step) {
        std::string doc = docs[step % docs.size()];
        std::vector<int> tokens = {BOS};
        for (char ch : doc) {
            auto it = std::find(uchars.begin(), uchars.end(), ch);
            tokens.push_back(std::distance(uchars.begin(), it));
        }
        tokens.push_back(BOS);

        int n = std::min(block_size, (int)tokens.size() - 1);
        std::vector<std::vector<Vec1D>> keys(n_layer), values(n_layer);
        Vec1D losses;

        for (int pos_id = 0; pos_id < n; ++pos_id) {
            int token_id = tokens[pos_id];
            int target_id = tokens[pos_id + 1];
            Vec1D logits = gpt(token_id, pos_id, keys, values);
            Vec1D probs = softmax_val(logits);
            losses.push_back(-log(probs[target_id]));
        }

        ValuePtr loss = make_val(0.0);
        for (const auto& l : losses) loss = loss + l;
        loss = loss * (1.0 / n);

        loss->backward();

        double lr_t = learning_rate * (1.0 - (double)step / num_steps);
        for (size_t i = 0; i < params.size(); ++i) {
            m[i] = beta1 * m[i] + (1.0 - beta1) * params[i]->grad;
            v[i] = beta2 * v[i] + (1.0 - beta2) * (params[i]->grad * params[i]->grad);
            double m_hat = m[i] / (1.0 - std::pow(beta1, step + 1));
            double v_hat = v[i] / (1.0 - std::pow(beta2, step + 1));
            params[i]->data -= lr_t * m_hat / (std::sqrt(v_hat) + eps_adam);
            params[i]->grad = 0.0;
        }

        final_loss = loss->data;
        if ((step + 1) % 100 == 0 || step == 0)
            printf("step %4d / %4d | loss %.4f\n", step + 1, num_steps, final_loss);
    }

    // 8. Save GGUF
    std::cout << "\n[save] Saving model to " << gguf_path << " ...\n";
    save_gguf(gguf_path, state_dict, n_layer, n_embd, block_size, n_head,
              vocab_size, BOS, uchars, num_steps, final_loss);

    // 9. Load GGUF and run inference
    std::cout << "\n[>>] Loading model from " << gguf_path << " ...\n";
    GGUFReader reader;
    GGUFModel model = reader.load(gguf_path);

    predict_samples(model);

    return 0;
}