#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <gtkmm.h>
#include <thread>
#include <chrono>

// Function to get the current UFW rules
std::string getUfwRules() {
    std::string cmd = "sudo ufw status";
    std::string output = "";
    FILE *fp = popen(cmd.c_str(), "r");
    if (fp == nullptr) {
        std::cerr << "Error executing command.\n";
        return "";
    }
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        output += buffer;
    }
    pclose(fp);
    return output;
}

// Function to delete a UFW rule
void deleteUfwRule(const std::string &rule) {
    std::string cmd = "sudo ufw delete " + rule;
    system(cmd.c_str());
}

// Function to handle changes and delete new rule
void onRuleChangeDetected(Gtk::Window &window, Gtk::Label &label, const std::string &newRules) {
    label.set_text("A new UFW rule was detected. Deleting the rule...");
    window.show_all();

    // Extract the new rule from the output
    size_t pos = newRules.find(" added");
    if (pos != std::string::npos) {
        size_t start = newRules.rfind("\n", pos);
        start = (start == std::string::npos) ? 0 : start + 1;
        size_t end = newRules.find("\n", start);
        std::string rule = newRules.substr(start, end - start);
        deleteUfwRule(rule);
    }
}

// Function to monitor UFW rules and detect changes
void monitorUfwRules(Gtk::Window &window, Gtk::Label &label) {
    std::string prevRules = getUfwRules();
    while (true) {
        std::string newRules = getUfwRules();
        // Check if there are any changes
        if (newRules != prevRules) {
            // Alert window and delete new rule
            g_idle_add(
                [](gpointer data) -> gboolean {
                    auto *p = static_cast<std::pair<Gtk::Window*, Gtk::Label*>*>(data);
                    onRuleChangeDetected(*p->first, *p->second, getUfwRules());
                    delete p;
                    return FALSE; // one-time call
                },
                new std::pair<Gtk::Window*, Gtk::Label*>(&window, &label)
            );
        }
        // Update the previous rules
        prevRules = newRules;
        // Sleep for 1 second before checking again
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "24958202.ronnie.org.gtkmm.example");
    Gtk::Window window;
    window.set_title("UFW Rule Change Detector");
    Gtk::Label label("Monitoring UFW rules...");
    window.add(label);
    window.show_all();
    // Start monitoring UFW rules in a thread
    std::thread monitorThread(monitorUfwRules, std::ref(window), std::ref(label));
    monitorThread.detach();
    return app->run(window);
}