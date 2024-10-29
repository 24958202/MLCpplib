#include <gtkmm.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "/Users/dengfengji/ronnieji/MLCpplib-main/bitmap.xpm"
#include "/Users/dengfengji/ronnieji/MLCpplib-main/bitmap2.xpm"

// Forward declaration of the PreviewWindow class
class PreviewWindow;
class ExampleWindow : public Gtk::Window
{
public:
    ExampleWindow();
    virtual ~ExampleWindow();
    // Make GridColumns public
    class GridColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        GridColumns() { add(m_col_country); add(m_col_sales); }

        Gtk::TreeModelColumn<Glib::ustring> m_col_country;
        Gtk::TreeModelColumn<int> m_col_sales;
    };
    GridColumns m_GridColumns;  // Move to public
protected:
    // Signal handlers
    void on_menu_file_quit();
    void on_menu_view_refresh();
    void on_button_clicked();
    void on_button_preview();
    /*
        toolbar start
    */
     // Signal handlers
    void on_toolbar_button_clicked(const Glib::ustring& label);
    // Helper function to add a button to the toolbar
    void add_toolbar_button(const char* const* xpm_data, const Glib::ustring& label_text);
    /*
        toolbar end
    */
    //update status bar's time
    void update_time();
    // Subwidgets
    Gtk::Box m_MainBox; // Main vertical box
    Gtk::Toolbar m_Toolbar; // Toolbar for buttons
    // Child widgets
    Gtk::Box m_VBox;
    Gtk::Button m_Button;
    Gtk::Button m_ButtonPreview;
    // Menu
    Gtk::MenuBar m_MenuBar;
    Gtk::MenuItem m_MenuItem_File;
    Gtk::Menu m_Menu_File;
    Gtk::MenuItem m_MenuItem_Quit;
    Gtk::MenuItem m_MenuItem_View;
    Gtk::Menu m_Menu_View;
    Gtk::MenuItem m_MenuItem_Refresh;
    // Other widgets
    Gtk::ListBox m_ListBox;
    Gtk::ComboBoxText m_ComboBox;
    Gtk::TreeView m_TreeView;
    Gtk::ProgressBar m_ProgressBar;
    Gtk::Statusbar m_StatusBar;
    Gtk::Grid m_Grid;
    Gtk::Box m_OptionBox;
    Gtk::TreeView m_GridView;
    Glib::RefPtr<Gtk::ListStore> m_GridModel;
    guint m_ContextId;
    Glib::RefPtr<Gtk::ListStore> m_SharedGridModel;
    // Model for TreeView
    Glib::RefPtr<Gtk::TreeStore> m_RefTreeModel;
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns() { add(m_col_pixbuf); add(m_col_name); }

        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_col_pixbuf; // Pixbuf for actual pixmap
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    };
    ModelColumns m_Columns;
    /*
        status bar start
    */
    Gtk::Box m_BottomBox; // Box to hold bottom content
    Gtk::Label m_CompanyLabel; // Label to display the company name
    Gtk::Label m_TimeLabel; // Label to display the current time
        // Dispatcher for updating the time
    Glib::Dispatcher m_Dispatcher;
    // Thread for updating the time
    std::thread m_Thread;
    /*
        statusbar end
    */
private:
    bool m_Running; // Flag to control the running state of the thread
};
class PreviewWindow : public Gtk::Window
{
public:
    PreviewWindow(const Glib::RefPtr<Gtk::ListStore>& model, const ExampleWindow::GridColumns& columns);
protected:
    // Signal handlers
    void on_button_print();
    void on_button_exit();
    // Child widgets for preview
    Gtk::Box m_VBox;
    Gtk::TextView m_TextView;
    Gtk::Button m_ButtonPrint;
    Gtk::Button m_ButtonExit;
    Gtk::Grid m_Grid;
    // Data model and columns (shared)
    Glib::RefPtr<Gtk::ListStore> m_PreviewGridModel;
    const ExampleWindow::GridColumns& m_Columns;
    // Helper method to populate TextView as print preview
    void populate_text_view();
};
ExampleWindow::ExampleWindow()
    : m_VBox(Gtk::ORIENTATION_VERTICAL),
      m_Button("Click Me"),
      m_ButtonPreview("Preview & Print"),
      m_BottomBox(Gtk::ORIENTATION_HORIZONTAL), 
      m_Running(true)
{
    set_title("GTK Example");
    set_default_size(800, 600);
    add(m_VBox);
    // Menu setup
    m_MenuItem_File.set_label("File");
    m_MenuBar.append(m_MenuItem_File);
    m_MenuItem_Quit.set_label("Quit");
    m_MenuItem_Quit.signal_activate().connect(sigc::mem_fun(*this,
              &ExampleWindow::on_menu_file_quit));
    m_Menu_File.append(m_MenuItem_Quit);
    m_MenuItem_File.set_submenu(m_Menu_File);
    m_MenuItem_View.set_label("View");
    m_MenuBar.append(m_MenuItem_View);
    m_MenuItem_Refresh.set_label("Refresh");
    m_MenuItem_Refresh.signal_activate().connect(sigc::mem_fun(*this,
              &ExampleWindow::on_menu_view_refresh));
    m_Menu_View.append(m_MenuItem_Refresh);
    m_MenuItem_View.set_submenu(m_Menu_View);
    m_VBox.pack_start(m_MenuBar, Gtk::PACK_SHRINK);
    //toolbar setup
    add_toolbar_button(bitmap_xpm, "Button 1");
    add_toolbar_button(bitmap2_xpm, "Button 2");
    m_VBox.pack_start(m_Toolbar, Gtk::PACK_SHRINK);
    // List box setup
    Gtk::Label* label1 = Gtk::make_managed<Gtk::Label>("List Item 1");
    Gtk::Label* label2 = Gtk::make_managed<Gtk::Label>("List Item 2");
    m_ListBox.append(*label1);
    m_ListBox.append(*label2);
    // Combo box setup
    m_ComboBox.append("Option 1");
    m_ComboBox.append("Option 2");
    m_ComboBox.set_active(0);
    // Load the XPM images into Gdk::Pixbuf
    auto pixbuf1 = Gdk::Pixbuf::create_from_xpm_data(bitmap_xpm);
    auto pixbuf2 = Gdk::Pixbuf::create_from_xpm_data(bitmap2_xpm);
    // TreeView setup with hierarchical data
    m_RefTreeModel = Gtk::TreeStore::create(m_Columns);
    m_TreeView.set_model(m_RefTreeModel);
    // Add pixbuf and name columns
    m_TreeView.append_column("Icon", m_Columns.m_col_pixbuf);
    m_TreeView.append_column("Name", m_Columns.m_col_name);
    // Populate TreeStore with hierarchical data and icons
    Gtk::TreeModel::Row row = *(m_RefTreeModel->append());
    row[m_Columns.m_col_pixbuf] = pixbuf1; // Assign the first pixbuf
    row[m_Columns.m_col_name] = "Level 1";
    Gtk::TreeModel::Row childrow = *(m_RefTreeModel->append(row.children()));
    childrow[m_Columns.m_col_pixbuf] = pixbuf2; // Assign the second pixbuf
    childrow[m_Columns.m_col_name] = "Level 2.1";
    row = *(m_RefTreeModel->append());
    row[m_Columns.m_col_pixbuf] = pixbuf1;
    row[m_Columns.m_col_name] = "Level 2.2";
    childrow = *(m_RefTreeModel->append(row.children()));
    childrow[m_Columns.m_col_pixbuf] = pixbuf2;
    childrow[m_Columns.m_col_name] = "Level 3.1";
    row = *(m_RefTreeModel->append());
    row[m_Columns.m_col_pixbuf] = pixbuf1;
    row[m_Columns.m_col_name] = "Level 2.3";
    m_TreeView.set_size_request(200, 150);  // Ensure TreeView gets an appropriate size
    // Option Box setup
    m_OptionBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    Gtk::RadioButton::Group group;
    Gtk::RadioButton* option1 = Gtk::make_managed<Gtk::RadioButton>(group, "Option 1");
    Gtk::RadioButton* option2 = Gtk::make_managed<Gtk::RadioButton>(group, "Option 2");
    m_OptionBox.pack_start(*option1);
    m_OptionBox.pack_start(*option2);
    // Connect button signals
    m_Button.signal_clicked().connect(sigc::mem_fun(*this,
              &ExampleWindow::on_button_clicked));
    m_ButtonPreview.signal_clicked().connect(sigc::mem_fun(*this,
              &ExampleWindow::on_button_preview));
    // Grid setup with aligned components
    m_Grid.set_column_spacing(10);
    m_Grid.set_row_spacing(10);
    m_Grid.attach(m_ListBox, 0, 0, 1, 1);
    m_Grid.attach(m_ComboBox, 1, 0, 1, 1);
    m_Grid.attach(m_TreeView, 0, 1, 2, 1);
    m_Grid.attach(m_OptionBox, 2, 0, 1, 2);
    // Add new elements below the existing ones
    m_Grid.attach(m_Button, 0, 2, 1, 1);
    m_Grid.attach(m_ProgressBar, 1, 2, 2, 1);
    m_Grid.attach(m_ButtonPreview, 2, 2, 1, 1);
    // GridView setup
    m_SharedGridModel = Gtk::ListStore::create(m_GridColumns);
    m_GridModel = m_SharedGridModel;
    m_GridView.set_model(m_GridModel);
    m_GridView.append_column("Country", m_GridColumns.m_col_country);
    m_GridView.append_column("Sales", m_GridColumns.m_col_sales);
    // Populate GridView with fake data
    Gtk::TreeModel::Row grid_row = *(m_GridModel->append());
    grid_row[m_GridColumns.m_col_country] = "Argentina";
    grid_row[m_GridColumns.m_col_sales] = 2000;
    grid_row = *(m_GridModel->append());
    grid_row[m_GridColumns.m_col_country] = "Belgium";
    grid_row[m_GridColumns.m_col_sales] = 4500;
    grid_row = *(m_GridModel->append());
    grid_row[m_GridColumns.m_col_country] = "France";
    grid_row[m_GridColumns.m_col_sales] = 4400;
    grid_row = *(m_GridModel->append());
    grid_row[m_GridColumns.m_col_country] = "Germany";
    grid_row[m_GridColumns.m_col_sales] = 3350;
    grid_row = *(m_GridModel->append());
    grid_row[m_GridColumns.m_col_country] = "Spain";
    grid_row[m_GridColumns.m_col_sales] = 5600;
    m_Grid.attach(m_GridView, 0, 3, 3, 1);
    m_VBox.pack_start(m_Grid, Gtk::PACK_EXPAND_WIDGET);
    // Status bar setup
    m_ContextId = m_StatusBar.get_context_id("Status");
    m_StatusBar.push("Ready");
    m_VBox.pack_start(m_StatusBar, Gtk::PACK_SHRINK);
    /*
        status bar time & company display
    */
    // Set the company name and align it left
    m_CompanyLabel.set_text("24958202@qq.com");
    m_CompanyLabel.set_xalign(0.0); // Align text to the left
    // Add the company label to the bottom box
    m_BottomBox.pack_start(m_CompanyLabel, true, true, 0);
    // Align the time label to the right
    m_TimeLabel.set_xalign(1.0); // Align text to the right
    // Add the time label to the bottom box
    m_BottomBox.pack_end(m_TimeLabel, false, false, 0);
    // Add the bottom box to the main box
    m_MainBox.pack_end(m_BottomBox, Gtk::PACK_SHRINK);
    // Connect the dispatcher to the update_time function
    m_Dispatcher.connect(sigc::mem_fun(*this, &ExampleWindow::update_time));
    // Start a thread to update the time
    m_Thread = std::thread([this]() {
        while (m_Running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_Dispatcher.emit();
        }
    });
    m_VBox.pack_end(m_MainBox, Gtk::PACK_SHRINK);
    show_all_children();
}
void ExampleWindow::on_menu_file_quit()
{
    hide(); // Closes the application
}
void ExampleWindow::on_menu_view_refresh()
{
    m_StatusBar.push("Refreshed", m_ContextId);
}
void ExampleWindow::on_button_clicked()
{
    m_ProgressBar.set_fraction(0.5); // Example: set progress to 50%
    m_StatusBar.push("Button Clicked!", m_ContextId);
}
void ExampleWindow::on_button_preview()
{
    auto preview_window = new PreviewWindow(m_SharedGridModel, m_GridColumns);
    preview_window->set_transient_for(*this);
    preview_window->show();
}
PreviewWindow::PreviewWindow(const Glib::RefPtr<Gtk::ListStore>& model, const ExampleWindow::GridColumns& columns)
    : m_VBox(Gtk::ORIENTATION_VERTICAL),
      m_ButtonPrint("Print"),
      m_ButtonExit("Exit"),
      m_PreviewGridModel(model),
      m_Columns(columns)
{
    set_title("Print Preview");
    set_default_size(600, 400);
    add(m_VBox);
    // Setup TextView for print preview
    m_TextView.set_editable(false);
    m_TextView.set_wrap_mode(Gtk::WRAP_WORD);
    populate_text_view();
    // Connect buttons
    m_ButtonPrint.signal_clicked().connect(sigc::mem_fun(*this, 
        &PreviewWindow::on_button_print));
    m_ButtonExit.signal_clicked().connect(sigc::mem_fun(*this, 
        &PreviewWindow::on_button_exit));
    // Arrange grid layout
    m_Grid.set_column_spacing(10);
    m_Grid.set_row_spacing(10);
    m_Grid.attach(m_TextView, 0, 0, 2, 1);
    m_Grid.attach(m_ButtonPrint, 0, 1, 1, 1);
    m_Grid.attach(m_ButtonExit, 1, 1, 1, 1);
    m_VBox.pack_start(m_Grid, Gtk::PACK_EXPAND_WIDGET);
    show_all_children();
}
ExampleWindow::~ExampleWindow()
{
    // Stop the thread and wait for it to finish
    m_Running = false;
    if (m_Thread.joinable())
        m_Thread.join();
}
void ExampleWindow::add_toolbar_button(const char* const* xpm_data, const Glib::ustring& label_text)
{
    // Create a Pixbuf from the XPM data
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_xpm_data(xpm_data);
    // Create an image from the Pixbuf
    auto image = Gtk::manage(new Gtk::Image(pixbuf));
    // Create a vertical box to hold both the image and label
    auto vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    vbox->pack_start(*image, Gtk::PACK_SHRINK);
    // Create a label and add it to the box
    auto label = Gtk::manage(new Gtk::Label(label_text));
    vbox->pack_start(*label, Gtk::PACK_SHRINK);
    // Create an event box to capture clicks
    auto eventBox = Gtk::manage(new Gtk::EventBox());
    eventBox->add(*vbox);
    eventBox->set_tooltip_text(label_text);
    // Connect the click signal to the event box
    eventBox->signal_button_press_event().connect([this, label_text](GdkEventButton* /*button_event*/) {
        on_toolbar_button_clicked(label_text);
        return true;
    });
    // Create a ToolItem and add the event box to it
    auto toolItem = Gtk::manage(new Gtk::ToolItem());
    toolItem->add(*eventBox);
    // Add the tool item to the toolbar
    m_Toolbar.append(*toolItem);
}
void ExampleWindow::on_toolbar_button_clicked(const Glib::ustring& label)
{
    // Create and show a message dialog
    Gtk::MessageDialog dialog(*this, "Button Clicked", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
    dialog.set_secondary_text(label + " was clicked.");
    dialog.run();
}
void PreviewWindow::populate_text_view()
{
    auto buffer = m_TextView.get_buffer();
    Glib::ustring text;
    // Construct text representing grid data
    text += "Print Preview:\n\n";
    for(auto&& row : m_PreviewGridModel->children())
    {
        Glib::ustring country = row[m_Columns.m_col_country];
        int sales = row[m_Columns.m_col_sales];
        text += "Country: " + country + "\n";
        text += "Sales: " + Glib::ustring::format(sales) + "\n";
        text += "--------------------\n";
    }
    buffer->set_text(text);
}
void PreviewWindow::on_button_print()
{
    // Placeholder for actual print functionality
    std::cout << "Print functionality to be implemented." << std::endl;
}
void PreviewWindow::on_button_exit()
{
    hide();  // Close the preview window
}
void ExampleWindow::update_time()
{
    // Get current time and format it
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto time_str = oss.str();

    // Update the time label with the current time
    m_TimeLabel.set_text(time_str);
}
int main(int argc, char* argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");
    ExampleWindow window;
    return app->run(window);
}
