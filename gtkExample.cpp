#include <gtkmm.h>
#include <iostream>

#include "/Users/dengfengji/ronnieji/MLCpplib-main/bitmap.xpm"
#include "/Users/dengfengji/ronnieji/MLCpplib-main/bitmap2.xpm"

// Forward declaration of the PreviewWindow class
class PreviewWindow;

class ExampleWindow : public Gtk::Window
{
public:
    ExampleWindow();

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
      m_ButtonPreview("Preview & Print")
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
    m_VBox.pack_end(m_StatusBar, Gtk::PACK_SHRINK);

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

int main(int argc, char* argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    ExampleWindow window;

    return app->run(window);
}
