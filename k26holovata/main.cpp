#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include "structures.h"

using namespace std;

template <class T>
class Table {
private:
    map<int, int> idAndLine;
    map<int, bool> idAndStatus; // 0 <=> removed
    set<int> deletedRecords;
    int rowsCount = 0;
    string fileFlName;
    string fileIdName;
    string fileGarbageName;
    
    void writeRow(T newRow, int id) {
        fstream rowWriter;
        rowWriter.open(fileFlName, ios::out | ios::in | ios::binary); // opening file for reading and writing in binary mode
        if (rowWriter.bad()) {
            cout << "ERROR. Could not open file\n\n";
            return;
        }
        rowWriter.seekp(sizeof(T) * id, ios::beg); // positioning the cursor to the (sizeof(T) * index)-th byte from the beginning
        rowWriter.write(reinterpret_cast<char*>(&newRow), sizeof(T)); // writing reinterpret_cast<char*>(&newRow); its size = sizeof(T)

        idAndLine[newRow.getStructureId()] = id;
        idAndStatus[newRow.getStructureId()] = true;
        rowWriter.close();
    }

    T readRow(int id) {
        T structure;
        ifstream rowReader;
        rowReader.open(fileFlName, ios::in | ios::binary); // opening file for reading in binary mode
        if (rowReader.bad()) {
            cout << "ERROR. Could not open file\n\n";
            return T();
        }
        rowReader.ignore(sizeof(T) * id); // ignoring first (sizeof(T) * id) bytes
        rowReader.read((char*)&structure, sizeof(T)); // reading sizeof(T) bytes into (char*)&structure
        rowReader.close();
        return structure;
    }

public:
    Table(string flName, string idName, string garbageName) {
        fileFlName = flName;
        fileIdName = idName;
        fileGarbageName = garbageName;

        ifstream fileIdReader; //reading from db files
        fileIdReader.open(fileIdName);
        int key, value;
        bool status;
        while (fileIdReader >> key >> value >> status) {
            idAndLine[key] = value;
            idAndStatus[key] = status;
        }
        fileIdReader.close();

        ifstream fileGarbageReader; //reading from garb file
        fileGarbageReader.open(fileGarbageName);
        int delId;
        while (fileGarbageReader >> delId) {
            deletedRecords.insert(delId);
        }
        fileGarbageReader.close();
    }

    ~Table() {
        ofstream fileIdWriter; //writing to db files
        fileIdWriter.open(fileIdName);
        for (auto& [key, value] : idAndLine) {
            fileIdWriter << key << " " << value << " " << idAndStatus[key] << endl;
        }
        fileIdWriter.close();

        ofstream fileGarbageWriter; //writing to garb file
        fileGarbageWriter.open(fileIdName);
        for (auto dr : deletedRecords) {
            fileGarbageWriter << dr << endl;
        }
        fileGarbageWriter.close();
    }

    T getRow(int id) {
        if (!idAndStatus[id]) return T(-1);
        return readRow(idAndLine[id]);
    }

    bool insertRow(T structure) {
        int id = structure.getStructureId();
        if (idAndStatus[id] && idAndStatus.count(id) > 0) return 0;
        else {
            if (rowsCount > 100) {
                auto newIndex = *(deletedRecords.begin());
                id = newIndex;
                deletedRecords.erase(id);
            }
            writeRow(structure, id);
            rowsCount += 1;
            return 1;
        }
    }

    T findRow(int id) {
        if (!idAndStatus[id] || idAndStatus.count(id) == 0) return T(-1);
        return readRow(idAndLine[id]);
    }

    T printRow(int id) {
        return readRow(idAndLine[id]);
    }

    void deleteRow(int id) {
        if (!idAndStatus[id] || idAndStatus.count(id) == 0) cout << "ERROR. This record has been removed" << endl;
        idAndStatus[id] = false;
        deletedRecords.insert(idAndLine[id]);
        rowsCount -= 1;
    }

    bool updateRow(T newRow) {
        if (!idAndStatus[newRow.getStructureId()] || idAndStatus.count(newRow.getStructureId()) == 0) return 0;
        writeRow(newRow, idAndLine[newRow.getStructureId()]);
        return 1;
    }

    int countRows() {
        return rowsCount;
    }

    void printTable() {
        for (auto& [key, value] : idAndStatus) {
            printRow(key).printInfo();
            cout << idAndStatus[key] << endl;
        }
    }
};

void clearFile(string fileFlName) {
    ofstream ofs; //opening file for writing
    ofs.open(fileFlName, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

class Database {
private:
    Table<Customer> customersDatabase;
    Table<Cafe> cafesDatabase;
    string cafeFlFile;
    string cafeIdFile;
    string cafeGarbageFile;
    string customerFlFile;
    string customerIdFile;
    string customerGarbageFile;
    
public:
    Database(string cafeFile, string cafeIDFile, string cafeGarbFile, string custFile, string custIdFile, string custGarbFile) :
        cafesDatabase(cafeFile, cafeIDFile, cafeGarbFile), customersDatabase(custFile, custIdFile, custGarbFile) {
        cafeFlFile = cafeFile;
        cafeIdFile = cafeIDFile;
        cafeGarbageFile = cafeGarbFile;
        customerFlFile = custFile;
        customerIdFile = custIdFile;
        customerGarbageFile = custGarbFile;
    }

    bool deleteCafe(int id) {
        Cafe cafe = cafesDatabase.findRow(id);
        if (cafe.getStructureId() == -1) return 0; // if there is no such cafe
        else {
            if (cafe.firstCustId != -1) { //if there are customers
                while (cafe.firstCustId != -1) {
                    Customer cust = customersDatabase.findRow(cafe.firstCustId);
                    cafe.firstCustId = cust.nextId;
                    customersDatabase.deleteRow(cust.getStructureId()); //deleting all customers from first to last
                }
            }
            cafesDatabase.deleteRow(id);
            return 1;
        }
    }

    bool deleteCustomer(int id) {
        Customer cust = customersDatabase.findRow(id);
        if (cust.getStructureId() == -1) return 0;

        Cafe cafe = cafesDatabase.findRow(cust.cafeId);
        if (cust.prevId != -1) { //if not the first customer
            Customer prevCust = customersDatabase.findRow(cust.prevId); 
            prevCust.nextId = cust.nextId;
            customersDatabase.updateRow(prevCust); //updating previous customer 
        }
        else {
            cafe.firstCustId = cust.nextId;
        }

        if (cust.nextId != -1) { //if not the last customer
            Customer nextCust = customersDatabase.findRow(cust.nextId);
            nextCust.prevId = cust.prevId;
            customersDatabase.updateRow(nextCust); //updating next customer 
        }
        else {
            cafe.lastCustId = cust.prevId;
        }
        customersDatabase.deleteRow(id);
        return 1;
    }

    bool insertCustomer(Customer cust, int id, int cafeId) {
        Cafe cafe = cafesDatabase.findRow(cafeId);
        if (cafe.firstCustId == -1) { //if there are no customers
            cafe.firstCustId = id;
            cafe.lastCustId = id;
        }
        else {
            cust.prevId = cafe.lastCustId; //inserting in the end
            Customer prevCust = customersDatabase.findRow(cafe.lastCustId);
            prevCust.nextId = id;
            cafe.lastCustId = id;
            customersDatabase.updateRow(prevCust); //updating previous customer
        }
        cafesDatabase.updateRow(cafe);
        bool isSuccessful = customersDatabase.insertRow(cust);
        return isSuccessful;
    }

    void consoleInterface() {
        string cmd;
        map<string, int> mapping;
        mapping["get-m"] = 11;
        mapping["get-s"] = 12;
        mapping["del-m"] = 21;
        mapping["del-s"] = 22;
        mapping["update-m"] = 31;
        mapping["update-s"] = 32;
        mapping["insert-m"] = 41;
        mapping["insert-s"] = 42;
        mapping["calc-m"] = 51;
        mapping["calc-s"] = 52;
        mapping["ut-m"] = 61;
        mapping["ut-s"] = 62;
        mapping["exit"] = 7;

        while (cin >> cmd) {
            Structure s;
            int id;

            Cafe cafe;
            int cafeId;
            char cafeName[30];
            char address[100];

            Customer cust;
            int custId;
            char custName[30];
            char phoneNumber[15];

            bool isSuccessful;
            switch (mapping[cmd]) {

            case(11): //get-m
                cin >> id;
                s = cafesDatabase.printRow(id);
                if (s.getStructureId() == -1) {
                    cout << "ERROR. There is no Cafe with this ID\n\n";
                    continue;
                }
                cout << "Record info:\n";
                s.printInfo();
                cout << endl;
                break;

            case(12): //get-s
                cin >> id;
                s = customersDatabase.getRow(id);
                if (s.getStructureId() == -1) {
                    cout << "ERROR. There is no Customer with this ID\n\n";
                    continue;
                }
                cout << "Record info:\n";
                s.printInfo();
                cout << endl;
                break;

            case(21): //del-m
                cout << "Enter ID of a Cafe to be deleted:\n";
                cin >> id;
                isSuccessful = deleteCafe(id);
                if (isSuccessful) cout << "Cafe " << id << " has been deleted\n\n";
                else cout << "ERROR. There is no Cafe with this ID\n\n";
                break;

            case(22): //del-s
                cout << "Enter ID of a Customer to be deleted:\n";
                cin >> id;
                isSuccessful = deleteCustomer(id);
                if (isSuccessful) cout << "Customer " << id << " has been deleted\n\n";
                else cout << "ERROR. There is no Customer with this ID\n\n";
                break;

            case(31): //update-m
                cout << "Enter new values for a Cafe:\n";
                cin >> id >> cafeName >> address;
                cafesDatabase.updateRow(Cafe(id, cafeName, address));
                cout << "Cafe " << id << " has been updated\n\n";
                break;

            case(32): //update-s
                cout << "Enter new values for a Customer:\n";
                cin >> id >> cafeId >> custName >> phoneNumber;
                if (cafesDatabase.findRow(cafeId).getStructureId() == -1) {
                    cout << "ERROR. There is no Cafe with this ID (" << cafeId << " is incorrect Cafe ID)\n\n";
                    continue;
                }
                isSuccessful = customersDatabase.updateRow(Customer(id, cafeId, custName, phoneNumber));
                if (isSuccessful) cout << "Customer " << id << " has been updated\n\n";
                else cout << "ERROR. There is no Customer with this ID\n\n";
                break;

            case(41): //insert-m
                cout << "Enter values for a new Cafe:\n";
                cin >> id >> cafeName >> address;
                isSuccessful = cafesDatabase.insertRow(Cafe(id, cafeName, address));
                if (isSuccessful) cout << "New Cafe has been inserted\n\n";
                else cout << "ERROR. Could not insert the Cafe\n\n";
                break;

            case(42): //insert-s
                cout << "Enter values for a new Customer:\n";
                cin >> id >> cafeId >> custName >> phoneNumber;
                if (cafesDatabase.findRow(cafeId).getStructureId() == -1) {
                    cout << "ERROR. There is no Cafe with this ID (" << cafeId << " is incorrect Cafe ID)\n\n";
                    continue;
                }
                cust = Customer(id, cafeId, custName, phoneNumber);
                isSuccessful = insertCustomer(cust, id, cafeId);
                if (isSuccessful) cout << "New Customer has been inserted\n\n";
                else cout << "ERROR. Could not insert the Customer\n\n";
                break;

            case(51): //calc-m
                cout << "Number of active Cafes in the Database: " << cafesDatabase.countRows() << endl << endl;
                break;

            case(52): //calc-s
                cout << "Number of active Customers in the Database: " << customersDatabase.countRows() << endl << endl;
                break;

            case(61): //ut-m
                cout << "MASTER TABLE (CAFES)" << endl;
                cout << "---------------------------------------------------------------------------" << endl;
                cout << "ID | Name | Address | First Customer | Last Customer | Status (0 - removed)" << endl;
                cout << "---------------------------------------------------------------------------" << endl;
                cafesDatabase.printTable();
                cout << "---------------------------------------------------------------------------\n" << endl;
                break;

            case(62): //ut-s
                cout << "SLAVE TABLE (CUSTOMERS)" << endl;
                cout << "---------------------------------------------------------" << endl;
                cout << "ID | Cafe ID | Name | Phone Number | Status (0 - removed)" << endl;
                cout << "---------------------------------------------------------" << endl;
                customersDatabase.printTable();
                cout << "---------------------------------------------------------\n" << endl;
                break;

            case(7): //exit
                exit(0);
                break;

            default:
                cout << "ERROR. Unknown command\n\n";
            }
        }
    }
};

int main() {
    clearFile("Cafe.fl");
    clearFile("Cafe.id");
    clearFile("Customer.fl");
    clearFile("Customer.id");
    
    Database db("Cafe.fl", "Cafe.id", "CafeGarbage.txt", "Customer.fl", "Customer.id", "CustomerGarbage.txt");
    db.consoleInterface();

    return 0;
}
