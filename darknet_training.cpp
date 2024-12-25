#include <iostream>  
#include <stdio.h>  
#include <cstdlib>  
#include <cstring>  
#include <pthread.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include "darknet.h"  

int main(int argc, char **argv)  
{  
    // Check if enough arguments are supplied  
    if (argc < 5) {  
        std::cerr << "Usage: " << argv[0] << " <train_images> <cfgfile> <weightfile(optional)> <output_weightfile>" << std::endl;  
        return 1;  
    }  
    char *train_images = argv[1];    // Path to training images list  
    char *cfgfile = argv[2];         // Path to the config file  
    char *weightfile = argv[3];      // Path to the weights file (optional)  
    char *outfile = argv[4];         // Path to save the trained weights  
    // Check if input files exist  
    if (access(train_images, F_OK) != 0) {  
        std::cerr << "Error: Training images file does not exist: " << train_images << std::endl;  
        return 1;  
    }  
    if (access(cfgfile, F_OK) != 0) {  
        std::cerr << "Error: Config file does not exist: " << cfgfile << std::endl;  
        return 1;  
    }  
    if (weightfile && access(weightfile, F_OK) != 0) {  
        std::cerr << "Error: Weight file does not exist: " << weightfile << std::endl;  
        return 1;  
    }  
    // Initialize the network  
    network *net = load_network(cfgfile, weightfile, 0);  
    // Determine the number of classes from the network layers  
    int num_classes = 0;  
    for (int i = 0; i < net->n; ++i) {  
        layer l = net->layers[i];  
        if (l.type == YOLO || l.type == REGION) {  
            num_classes = l.classes;  
            break; // Assuming all detection layers have the same number of classes  
        }  
    }  
    if (num_classes == 0) {  
        std::cerr << "Error: No YOLO or REGION layer found in the network to determine number of classes." << std::endl;  
        free_network(net);  
        return 1;  
    }  
    // Auto-detect the number of CPU cores  
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);  
    if (num_cpus <= 0) {  
        std::cerr << "Error: Failed to detect the number of CPUs." << std::endl;  
        free_network(net);  
        return 1;  
    }  
    // Prepare data loading arguments  
    load_args args = {0};  
    args.w = net->w;  
    args.h = net->h;  
    args.n = net->batch * net->subdivisions; // Number of images per batch  
    args.m = 0; // Total number of images, we'll set it later  
    args.classes = num_classes;                   // Number of classes  
    args.type = DETECTION_DATA;  
    args.threads = num_cpus; // Use the detected number of CPUs for data loading threads  
    args.angle = net->angle;  
    args.exposure = net->exposure;  
    args.saturation = net->saturation;  
    args.hue = net->hue;  
    // Load list of image paths  
    list *plist = get_paths(train_images);  
    if (!plist) {  
        std::cerr << "Error: Could not open file containing training image paths." << std::endl;  
        free_network(net);  
        return 1;  
    }  
    char **paths = (char **)list_to_array(plist);  
    int N = plist->size;  
    if (N == 0) {  
        std::cerr << "Error: No images found in the training paths file." << std::endl;  
        free_list(plist);  
        free_network(net);  
        return 1;  
    }  
    args.paths = paths;  
    args.m = N;  
    // Load labels if needed  
    char **labels = get_labels("/home/ronnieji/ronnieji/kaggle/data/obj.names"); // Path to a file containing list of labels  
    if (!labels) {  
        std::cerr << "Error: Could not load labels from the specified file." << std::endl;  
        free_list(plist);  
        free(paths);  
        free_network(net);  
        return 1;  
    }  
    args.labels = labels;  
    // Variables to handle data loading  
    data train;  
    args.d = &train;  
    // Start loading data in a new thread  
    pthread_t load_thread = load_data_in_thread(args);  
    int max_batches = net->max_batches;  
    int epoch = 0;  
    int current_batch = get_current_batch(net);  
    while (current_batch < max_batches) {  
        // Wait for data to be loaded  
        pthread_join(load_thread, 0);  
        // Train the network with the loaded data  
        float loss = train_network(net, train);  
        // Output training information  
        std::cout << "Epoch: " << epoch << " Batch: " << current_batch << " Loss: " << loss << std::endl;  
        // Free the data  
        free_data(train);  
        // Update the current batch and epoch  
        current_batch = get_current_batch(net);  
        if (current_batch % (args.m / args.n) == 0) {  
            epoch++;  
        }  
        // Start loading the next batch  
        args.d = &train;  
        load_thread = load_data_in_thread(args);  
        // Save weights periodically  
        if (current_batch % 1000 == 0) {  
            char backup_file[256];  
            if (snprintf(backup_file, sizeof(backup_file), "%s_backup_%d.weights", outfile, current_batch) >= sizeof(backup_file)) {  
                std::cerr << "Error: Backup file path is too long." << std::endl;  
                break;  
            }  
            save_weights(net, backup_file);  
        }  
    }  
    // Save the final weights  
    save_weights(net, outfile);  
    // Free resources  
    free_network(net);  
    free_list(plist); // Free the list of paths  
    free(paths);      // Free the array of paths  
    if (labels) {  
        for (int i = 0; i < num_classes; ++i) {  
            free(labels[i]);  
        }  
        free(labels);  
    }  
    return 0;  
}