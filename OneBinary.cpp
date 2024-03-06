#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <X11/xpm.h>
#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pixmap.H>
#include <FL/fl_file_chooser.H>  // Include for file chooser dialog
#include <FL/fl_ask.H>
#include <FL/Fl_Multi_Browser.H>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <map>
#include <unordered_map>
#include "../lib/nemslib.h"

#include "icons/confirm.xpm"
#include "icons/exit.xpm"
#include "icons/app_iocn.xpm"

Fl_Double_Window win(625, 330, "::Binary library generator::");
Fl_Input *input;
Fl_Box *text;
Fl_Multi_Browser *listbox;

void addListBoxStatus(const std::string& input_str){
    listbox->add(input_str.c_str());
    if(listbox->size()>5){
        listbox->remove(1);
    }
}
void removeComs(){
    delete input;
    delete text;
    delete listbox;
}
void create_bi_2pair_txt(const std::string& file_input_path){
    std::string f_out_path_from_input_path;
    std::string input_file_name;
    std::string output_file_path;
    std::string strMsg;
    nlp_lib nl_j;
    if(file_input_path.empty()){
        strMsg = "create_bi_2pair_txt:Input string is empty!";
        fl_message("%s",strMsg.c_str());
        return;
    }
    if(!std::filesystem::exists(file_input_path)){
        strMsg = "Could not find "+ file_input_path +" file!";
        fl_message("%s",strMsg.c_str());
        return;
    }
    output_file_path = file_input_path;
    std::vector<std::string> split_names;
    boost::split(split_names,output_file_path,boost::is_any_of("/"));
    if(!split_names.empty()){
        input_file_name = split_names.back();
        std::vector<std::string> get_file_name;
        boost::split(get_file_name,input_file_name,boost::is_any_of("."));
        if(!get_file_name.empty()){
            boost::algorithm::replace_all(output_file_path,input_file_name,get_file_name[0] + ".bin");
        }
    }
    std::ifstream inFile(file_input_path);
    if(!inFile.is_open()){
        strMsg = "create_bi_2pair_txt:Failed to open file: " + file_input_path;
        fl_message("%s",strMsg.c_str());
        return;
    }
    /*
        read the text content from the file
    */
    std::string line;
    std::vector<std::string> raw_txt_content;
    while(std::getline(inFile, line)){
        raw_txt_content.push_back(line); //+ "^~&";
    }
    inFile.close();
    if(!raw_txt_content.empty()){
        nl_j.WriteBinaryOne_from_std(raw_txt_content,output_file_path);
    }
    for(const auto& rt : raw_txt_content){
        strMsg = "Reading: ";
        strMsg.append(rt);
        addListBoxStatus(strMsg);
    }
    strMsg = "Successfully created the binary file in: " + output_file_path;
    fl_message("%s",strMsg.c_str());
}
void button_callback(Fl_Widget* widget, void* data) {
    // Add your file selection logic here
    const char* selectedFile = fl_file_chooser("Select File", "*", NULL);
    if (selectedFile != NULL) {
        // File selection successful, handle the selected file
        input->value(selectedFile);  // Display the selected file path in the input field
    } else {
        // File selection canceled or failed, handle the error or cancellation
        std::string strMsg = "You did not select any file!";
        fl_message("%s",strMsg.c_str());
    }
}
void btConfirm(Fl_Widget*, void* data) {
    create_bi_2pair_txt(input->value());
}
void btExit(Fl_Widget*, void* data) {
    win.hide();  // Close the window
    removeComs();
    exit(0);
}
int main(int argc, char **argv) {
    
    text = new Fl_Box(10, 10, 250, 30, "Please select text file location:");
    text->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    input = new Fl_Input(10, 50, 250, 30, "");//"File Location:"
    listbox = new Fl_Multi_Browser(10,90,575,150);

    Fl_Button *button = new Fl_Button(270, 50, 120, 30, "Browse");
    button->callback(button_callback);

    Fl_Button *bt_confirm = new Fl_Button(420, 275, 80, 35, "Confirm");
    Fl_Pixmap *btconfirm_icon = new Fl_Pixmap(confirm_xpm);
    bt_confirm->image(btconfirm_icon); // Set confirm button icon
    bt_confirm->callback(btConfirm);

    Fl_Button *bt_exit = new Fl_Button(505, 275, 80, 35, "Exit");
    Fl_Pixmap *btexit_icon = new Fl_Pixmap(exit_xpm);
    bt_exit->image(btexit_icon); // Set exit button icon
    bt_exit->callback(btExit);

    win.size_range(win.w(), win.h(), win.w(), win.h());  // Fix the window size
    win.end();
    win.resizable(&win);
    Fl_Pixmap* winicon = new Fl_Pixmap(app_iocn_xpm);
    win.icon(winicon);
    win.show(argc, argv);
    win.position((Fl::w()-win.w())/2, (Fl::h()-win.h())/2);

    return Fl::run();
}
