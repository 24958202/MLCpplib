#include <iostream>
#include <vector>
#include <string>
#include <dlib/sqlite.h>
#include <dlib/matrix.h>
#include "nemslib.h"
void insert_sqlite() {  
    dlib::database db("/Users/dengfengji/ronnieji/libs/db/LLM.db");  
    // Insert data into the table  
    dlib::statement st(db, "INSERT INTO TT (strAA, strTT) VALUES (?, ?)");  
    std::string strName = "Jack";  
    std::string strAddress = "No#55, main ave.";  
    st.bind(1, strName);  
    st.bind(2, strAddress);  
    st.exec();  
    // Select and display data from the table  
    dlib::statement st2(db, "SELECT * FROM TT");  
    st2.exec();  
    while (st2.move_next()) {  
        std::string strname, straddress;  
        st2.get_column(1, strname);  
        st2.get_column(2, straddress);  
        std::cout << strname << " " << straddress << std::endl;  
    }  
    dlib::statement st3(db,"update TT set strTT=? where strAA=?");
    std::string strtt, straa;
    strtt = "#99,Ocen str.";
    straa = "Jack";
    st3.bind(1,strtt);
    st3.bind(2,straa);
    st3.exec();
    // Select and display data from the table  
    dlib::statement st4(db, "SELECT * FROM TT");  
    st4.exec();  
    while (st4.move_next()) {  
        std::string strname, straddress;  
        st4.get_column(1, strname);  
        st4.get_column(2, straddress);  
        std::cout << strname << " " << straddress << std::endl;  
    }  
    // dlib::statement st5(db, "delete FROM TT");  
    // st5.exec();  
}
void db_exec(){
    std::vector<std::vector<std::string>> dbresult;
	SQLite3Library db("/Users/dengfengji/ronnieji/libs/db/LLM.db");
	// Inside db_exec function  
    db.connect();  
    db.executeQuery("insert into TT(strAA,strTT) values('Tom', '123 main str.')", dbresult);  
    std::cout << "Successfully insert into the database." << std::endl;  

    // Fetch and display inserted data  
    dbresult.clear();  
    db.executeQuery("select * from TT", dbresult);  
    for (const auto& res : dbresult) {  
        std::cout << res[1] << " === " << res[2] << std::endl;  
    }  

    // Update statement  
    db.executeQuery("update TT set strTT='555 mahaa str.' where strAA='Tom'", dbresult);  
    dbresult.clear();  

    // Fetch and display updated data  
    db.executeQuery("select * from TT", dbresult);  
    for (const auto& res : dbresult) {  
        std::cout << res[1] << " === " << res[2] << std::endl;  
    }  

    db.disconnect();  // Disconnect after all operations  
    std::cout << "Successfully saved." << std::endl;
}
int main(){
    db_exec();
}