#include <iostream>
#include <string>

using namespace std;

class Structure {
public:
    virtual int getStructureId() { return -1; };
    virtual void printInfo() {};
};

class Customer : public Structure { //slave
private:
    char customerName[40];
    char phoneNumber[15];

public:
    int custId;
    int cafeId;
    int prevId = -1;
    int nextId = -1;
    
    Customer(int custId, int cafeId, char customerName[], char phoneNumber[]) {
        this->custId = custId;
        this->cafeId = cafeId;
        strcpy(this->customerName, customerName);
        strcpy(this->phoneNumber, phoneNumber);
    };

    Customer(int custId) {
        this->custId = custId;
    }

    Customer() {}

    void printInfo() {
        cout << custId << " | " << cafeId << " | " << customerName << " | " << phoneNumber << " | ";
    }

    int getStructureId() {
        return custId;
    };

    int getCafeId() {
        return cafeId;
    }
};

class Cafe : public Structure { //master
private:
    char cafeName[30];
    char cafeAddress[100];

public:
    int cafeId;
    int firstCustId = -1;
    int lastCustId = -1;

    Cafe(int cafeId, char cafeName[], char cafeAddress[]) {
        this->cafeId = cafeId;
        strcpy(this->cafeName, cafeName);
        strcpy(this->cafeAddress, cafeAddress);
    };

    Cafe(int cafeId, char cafeName[]) {
        this->cafeId = cafeId;
        strcpy(this->cafeName, cafeName);
    };

    Cafe(int cafeId) {
        this->cafeId = cafeId;
    };

    Cafe() {}

    void printInfo() {
        cout << cafeId << " | " << cafeName << " | " << cafeAddress << " | " << firstCustId << " | " << lastCustId << " | ";
    }

    int getStructureId() {
        return cafeId;
    }
};
