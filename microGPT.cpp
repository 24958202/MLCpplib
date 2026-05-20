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

// ----------------------------------------------------------------------------
// Autograd Engine (The 'Value' class)
// ----------------------------------------------------------------------------

struct Value;
using ValuePtr = std::shared_ptr<Value>;

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
                for (auto& child : v->_children) {
                    build_topo(child);
                }
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

// Operator Overloads to build the computation graph
ValuePtr operator+(const ValuePtr& a, const ValuePtr& b) {
    auto out = make_val(a->data + b->data);
    out->_children = {a, b};
    out->_local_grads = {1.0, 1.0};
    return out;
}
ValuePtr operator+(const ValuePtr& a, double b) { return a + make_val(b); }
ValuePtr operator+(double a, const ValuePtr& b) { return make_val(a) + b; }

ValuePtr operator*(const ValuePtr& a, const ValuePtr& b) {
    auto out = make_val(a->data * b->data);
    out->_children = {a, b};
    out->_local_grads = {b->data, a->data};
    return out;
}
ValuePtr operator*(const ValuePtr& a, double b) { return a * make_val(b); }
ValuePtr operator*(double a, const ValuePtr& b) { return make_val(a) * b; }

ValuePtr operator-(const ValuePtr& a) { return a * (-1.0); }
ValuePtr operator-(const ValuePtr& a, const ValuePtr& b) { return a + (-b); }
ValuePtr operator-(const ValuePtr& a, double b) { return a + (-b); }
ValuePtr operator-(double a, const ValuePtr& b) { return make_val(a) + (-b); }

ValuePtr pow(const ValuePtr& a, double b) {
    auto out = make_val(std::pow(a->data, b));
    out->_children = {a};
    out->_local_grads = {b * std::pow(a->data, b - 1)};
    return out;
}

ValuePtr operator/(const ValuePtr& a, const ValuePtr& b) { return a * pow(b, -1.0); }
ValuePtr operator/(const ValuePtr& a, double b) { return a * pow(make_val(b), -1.0); }
ValuePtr operator/(double a, const ValuePtr& b) { return make_val(a) * pow(b, -1.0); }

ValuePtr log(const ValuePtr& a) {
    auto out = make_val(std::log(a->data));
    out->_children = {a};
    out->_local_grads = {1.0 / a->data};
    return out;
}

ValuePtr exp(const ValuePtr& a) {
    auto out = make_val(std::exp(a->data));
    out->_children = {a};
    out->_local_grads = {std::exp(a->data)};
    return out;
}

ValuePtr relu(const ValuePtr& a) {
    auto out = make_val(std::max(0.0, a->data));
    out->_children = {a};
    out->_local_grads = {(a->data > 0) ? 1.0 : 0.0};
    return out;
}

// ----------------------------------------------------------------------------
// Globals & RNG Initialization
// ----------------------------------------------------------------------------

std::mt19937 gen(42); // Let there be order among chaos

using Vec1D = std::vector<ValuePtr>;
using Vec2D = std::vector<Vec1D>;

Vec2D matrix(int nout, int nin, double std = 0.08) {
    std::normal_distribution<double> d(0.0, std);
    Vec2D mat(nout, Vec1D(nin));
    for (int i = 0; i < nout; ++i) {
        for (int j = 0; j < nin; ++j) {
            mat[i][j] = make_val(d(gen));
        }
    }
    return mat;
}

// ----------------------------------------------------------------------------
// Model Architecture Functions
// ----------------------------------------------------------------------------

Vec1D linear(const Vec1D& x, const Vec2D& w) {
    Vec1D out(w.size());
    for (size_t i = 0; i < w.size(); ++i) {
        ValuePtr sum = make_val(0.0);
        for (size_t j = 0; j < x.size(); ++j) {
            sum = sum + (w[i][j] * x[j]);
        }
        out[i] = sum;
    }
    return out;
}

Vec1D softmax(const Vec1D& logits) {
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

Vec1D rmsnorm(const Vec1D& x) {
    ValuePtr ms = make_val(0.0);
    for (const auto& xi : x) ms = ms + (xi * xi);
    ms = ms / static_cast<double>(x.size());
    ValuePtr scale = pow(ms + 1e-5, -0.5);
    
    Vec1D out(x.size());
    for (size_t i = 0; i < x.size(); ++i) out[i] = x[i] * scale;
    return out;
}

// ----------------------------------------------------------------------------
// Main Routine
// ----------------------------------------------------------------------------

int main() {
    // 1. Let there be a Dataset
    if (!std::filesystem::exists("input.txt")) {
        std::cout << "Downloading dataset...\n";
        int res = std::system("curl -s -o input.txt https://raw.githubusercontent.com/karpathy/makemore/988aa59/names.txt");
        if(res != 0) { std::cerr << "Download failed.\n"; return 1; }
    }

    std::vector<std::string> docs;
    std::ifstream file("input.txt");
    std::string line;
    while (std::getline(file, line)) {
        // Strip basic whitespace/newlines
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (!line.empty()) docs.push_back(line);
    }
    std::shuffle(docs.begin(), docs.end(), gen);
    std::cout << "num docs: " << docs.size() << "\n";

    // 2. Let there be a Tokenizer
    std::set<char> unique_chars;
    for (const auto& d : docs) {
        for (char c : d) unique_chars.insert(c);
    }
    std::vector<char> uchars(unique_chars.begin(), unique_chars.end());
    int BOS = uchars.size();
    int vocab_size = uchars.size() + 1;
    std::cout << "vocab size: " << vocab_size << "\n";

    // 3. Initialize Parameters
    int n_layer = 1;
    int n_embd = 16;
    int block_size = 16;
    int n_head = 4;
    int head_dim = n_embd / n_head;

    std::map<std::string, Vec2D> state_dict;
    state_dict["wte"] = matrix(vocab_size, n_embd);
    state_dict["wpe"] = matrix(block_size, n_embd);
    state_dict["lm_head"] = matrix(vocab_size, n_embd);

    for (int i = 0; i < n_layer; ++i) {
        std::string prefix = "layer" + std::to_string(i) + ".";
        state_dict[prefix + "attn_wq"] = matrix(n_embd, n_embd);
        state_dict[prefix + "attn_wk"] = matrix(n_embd, n_embd);
        state_dict[prefix + "attn_wv"] = matrix(n_embd, n_embd);
        state_dict[prefix + "attn_wo"] = matrix(n_embd, n_embd);
        state_dict[prefix + "mlp_fc1"] = matrix(4 * n_embd, n_embd);
        state_dict[prefix + "mlp_fc2"] = matrix(n_embd, 4 * n_embd);
    }

    std::vector<ValuePtr> params;
    for (const auto& [name, mat] : state_dict) {
        for (const auto& row : mat) {
            for (const auto& p : row) params.push_back(p);
        }
    }
    std::cout << "num params: " << params.size() << "\n";

    // 4. The Model (GPT Forward Pass)
    auto gpt = [&](int token_id, int pos_id, std::vector<std::vector<Vec1D>>& keys, std::vector<std::vector<Vec1D>>& values) {
        Vec1D tok_emb = state_dict["wte"][token_id];
        Vec1D pos_emb = state_dict["wpe"][pos_id];
        Vec1D x(n_embd);
        for (int i = 0; i < n_embd; ++i) x[i] = tok_emb[i] + pos_emb[i];
        
        x = rmsnorm(x);

        for (int li = 0; li < n_layer; ++li) {
            std::string lpre = "layer" + std::to_string(li) + ".";
            
            // Multi-head Attention
            Vec1D x_residual = x;
            x = rmsnorm(x);
            Vec1D q = linear(x, state_dict[lpre + "attn_wq"]);
            Vec1D k = linear(x, state_dict[lpre + "attn_wk"]);
            Vec1D v = linear(x, state_dict[lpre + "attn_wv"]);
            
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
                
                Vec1D attn_weights = softmax(attn_logits);
                
                Vec1D head_out(head_dim, make_val(0.0));
                for (int j = 0; j < head_dim; ++j) {
                    ValuePtr sum = make_val(0.0);
                    for (size_t t = 0; t < v_h.size(); ++t) {
                        sum = sum + (attn_weights[t] * v_h[t][j]);
                    }
                    head_out[j] = sum;
                }
                x_attn.insert(x_attn.end(), head_out.begin(), head_out.end());
            }
            
            x = linear(x_attn, state_dict[lpre + "attn_wo"]);
            for (size_t i = 0; i < x.size(); ++i) x[i] = x[i] + x_residual[i];
            
            // MLP
            x_residual = x;
            x = rmsnorm(x);
            x = linear(x, state_dict[lpre + "mlp_fc1"]);
            for (size_t i = 0; i < x.size(); ++i) x[i] = relu(x[i]);
            x = linear(x, state_dict[lpre + "mlp_fc2"]);
            for (size_t i = 0; i < x.size(); ++i) x[i] = x[i] + x_residual[i];
        }

        return linear(x, state_dict["lm_head"]);
    };

    // 5. Optimizer Buffers (Adam)
    double learning_rate = 0.01, beta1 = 0.85, beta2 = 0.99, eps_adam = 1e-8;
    std::vector<double> m(params.size(), 0.0);
    std::vector<double> v(params.size(), 0.0);

    // 6. Training Loop
    int num_steps = 1000;
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
            Vec1D probs = softmax(logits);
            losses.push_back(-log(probs[target_id]));
        }

        ValuePtr loss = make_val(0.0);
        for (const auto& l : losses) loss = loss + l;
        loss = loss * (1.0 / n);

        loss->backward();

        // Adam Step
        double lr_t = learning_rate * (1.0 - (double)step / num_steps);
        for (size_t i = 0; i < params.size(); ++i) {
            m[i] = beta1 * m[i] + (1.0 - beta1) * params[i]->grad;
            v[i] = beta2 * v[i] + (1.0 - beta2) * (params[i]->grad * params[i]->grad);
            double m_hat = m[i] / (1.0 - std::pow(beta1, step + 1));
            double v_hat = v[i] / (1.0 - std::pow(beta2, step + 1));
            
            params[i]->data -= lr_t * m_hat / (std::sqrt(v_hat) + eps_adam);
            params[i]->grad = 0.0;
        }

        if ((step + 1) % 10 == 0 || step == 0) {
            printf("step %4d / %4d | loss %.4f\n", step + 1, num_steps, loss->data);
        }
    }

    // 7. Inference
    double temperature = 0.5;
    std::cout << "\n--- inference (new, hallucinated names) ---\n";
    
    for (int sample_idx = 0; sample_idx < 20; ++sample_idx) {
        std::vector<std::vector<Vec1D>> keys(n_layer), values(n_layer);
        int token_id = BOS;
        std::string sample = "";
        
        for (int pos_id = 0; pos_id < block_size; ++pos_id) {
            Vec1D logits = gpt(token_id, pos_id, keys, values);
            
            Vec1D scaled_logits(logits.size());
            for (size_t i = 0; i < logits.size(); ++i) {
                scaled_logits[i] = logits[i] / temperature;
            }
            Vec1D probs = softmax(scaled_logits);
            
            std::vector<double> prob_weights(probs.size());
            for (size_t i = 0; i < probs.size(); ++i) prob_weights[i] = probs[i]->data;
            
            std::discrete_distribution<int> dist(prob_weights.begin(), prob_weights.end());
            token_id = dist(gen);
            
            if (token_id == BOS) break;
            sample += uchars[token_id];
        }
        printf("sample %2d: %s\n", sample_idx + 1, sample.c_str());
    }

    return 0;
}