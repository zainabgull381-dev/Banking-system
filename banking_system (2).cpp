#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
using namespace std;

// ----------------------------------------
// PERMISSION BIT FLAGS
// ----------------------------------------
const unsigned int PERM_WITHDRAW = 1;  // 001
const unsigned int PERM_DEPOSIT  = 2;  // 010
const unsigned int PERM_TRANSFER = 4;  // 100
const unsigned int PERM_VIP      = 8;  // 1000

// ----------------------------------------
// TRANSACTION COMPRESSION
// ----------------------------------------
unsigned int encodeTx(unsigned int type, unsigned int amount) {
    return (type << 28) | (amount & 0x0FFFFFFF);
}

void decodeTx(unsigned int encoded, unsigned int &type, unsigned int &amount) {
    type   = (encoded >> 28) & 0xF;
    amount = encoded & 0x0FFFFFFF;
}

// ----------------------------------------
// ABSTRACT BASE CLASS
// ----------------------------------------
class Account {
protected:
    int          accountId;
    string       name;
    double       balance;
    unsigned int permissions;
    vector<double> transactions;
    double       monthlyTotals[12];

public:
    Account(int id, string n, double bal, unsigned int perm) {
        accountId   = id;
        name        = n;
        balance     = bal;
        permissions = perm;
        for (int i = 0; i < 12; i++) monthlyTotals[i] = 0;
    }

    virtual ~Account() {}

    virtual void deposit(double amount)  = 0;
    virtual void withdraw(double amount) = 0;
    virtual void saveToFile(ofstream &f) = 0;
    virtual string getType()             = 0;

    int    getId()      { return accountId; }
    string getName()    { return name; }
    double getBalance() { return balance; }

    // check permission using bitwise AND
    bool hasPermission(unsigned int flag) {
        return (permissions & flag) != 0;
    }

    // give permission using bitwise OR
    void givePermission(unsigned int flag) {
        permissions = permissions | flag;
    }

    // show account details
    void showAccount() {
        cout << "\n==============================\n";
        cout << "Account ID : " << accountId   << "\n";
        cout << "Name       : " << name         << "\n";
        cout << "Type       : " << getType()    << "\n";
        cout << "Balance    : $" << fixed << setprecision(2) << balance << "\n";
        cout << "Permissions: " << permissions  << "\n";
        cout << "------------------------------\n";
        cout << "Transaction History:\n";
        if (transactions.size() == 0) {
            cout << "  No transactions yet.\n";
        } else {
            for (int i = 0; i < (int)transactions.size(); i++) {
                if (transactions[i] >= 0)
                    cout << "  [" << i+1 << "] Deposit  : $" << transactions[i] << "\n";
                else
                    cout << "  [" << i+1 << "] Withdraw : $" << -transactions[i] << "\n";
            }
        }
        cout << "==============================\n";
    }

    // show monthly summary using array
    void showMonthlySummary() {
        string months[12] = {"Jan","Feb","Mar","Apr","May","Jun",
                             "Jul","Aug","Sep","Oct","Nov","Dec"};
        cout << "\n--- Monthly Summary ---\n";
        for (int i = 0; i < 12; i++) {
            if (monthlyTotals[i] != 0)
                cout << months[i] << " : $" << monthlyTotals[i] << "\n";
        }
    }

    // add transaction to history
    void addTransaction(double amount, int month) {
        transactions.push_back(amount);
        if (month >= 0 && month < 12)
            monthlyTotals[month] += amount;

        // demonstrate compression
        unsigned int type   = (amount >= 0) ? 1 : 2;
        unsigned int amt    = (unsigned int)(amount >= 0 ? amount : -amount);
        unsigned int enc    = encodeTx(type, amt);
        unsigned int dtype, damt;
        decodeTx(enc, dtype, damt);
        cout << "  [Compressed] encoded=" << enc
             << " decoded: type=" << dtype << " amount=$" << damt << "\n";
    }
};

// ----------------------------------------
// SAVINGS ACCOUNT
// ----------------------------------------
class SavingsAccount : public Account {
private:
    double interestRate;

public:
    SavingsAccount(int id, string n, double bal, unsigned int perm, double rate)
        : Account(id, n, bal, perm) {
        interestRate = rate;
    }

    string getType() { return "Savings"; }

    void deposit(double amount) {
        // check permission using bitwise AND
        if (!hasPermission(PERM_DEPOSIT)) {
            cout << "  [DENIED] No deposit permission!\n";
            return;
        }
        if (amount <= 0) {
            cout << "  [ERROR] Amount must be positive!\n";
            return;
        }
        balance = balance + amount;
        addTransaction(amount, 0);
        cout << "  [OK] Deposited $" << amount << "  New Balance: $" << balance << "\n";
    }

    void withdraw(double amount) {
        // check permission using bitwise AND
        if (!hasPermission(PERM_WITHDRAW)) {
            cout << "  [DENIED] No withdraw permission!\n";
            return;
        }
        if (amount > balance) {
            cout << "  [DENIED] Not enough balance!\n";
            return;
        }
        balance = balance - amount;
        addTransaction(-amount, 0);
        cout << "  [OK] Withdrew $" << amount << "  New Balance: $" << balance << "\n";
    }

    void applyInterest() {
        double interest = balance * interestRate;
        balance = balance + interest;
        addTransaction(interest, 0);
        cout << "  [OK] Interest $" << interest << " added.\n";
    }

    void saveToFile(ofstream &f) {
        f << "ACCOUNT Savings\n";
        f << accountId << " " << name << " " << balance << " " << permissions << "\n";
        f << "TRANSACTIONS\n";
        for (int i = 0; i < (int)transactions.size(); i++)
            f << transactions[i] << "\n";
        f << "END\n";
    }
};

// ----------------------------------------
// CURRENT ACCOUNT
// ----------------------------------------
class CurrentAccount : public Account {
private:
    double overdraftLimit;

public:
    CurrentAccount(int id, string n, double bal, unsigned int perm, double overdraft)
        : Account(id, n, bal, perm) {
        overdraftLimit = overdraft;
    }

    string getType() { return "Current"; }

    void deposit(double amount) {
        if (!hasPermission(PERM_DEPOSIT)) {
            cout << "  [DENIED] No deposit permission!\n";
            return;
        }
        if (amount <= 0) {
            cout << "  [ERROR] Amount must be positive!\n";
            return;
        }
        balance = balance + amount;
        addTransaction(amount, 0);
        cout << "  [OK] Deposited $" << amount << "  New Balance: $" << balance << "\n";
    }

    void withdraw(double amount) {
        if (!hasPermission(PERM_WITHDRAW)) {
            cout << "  [DENIED] No withdraw permission!\n";
            return;
        }
        // overdraft check
        if (balance - amount < -overdraftLimit) {
            cout << "  [DENIED] Overdraft limit exceeded!\n";
            return;
        }
        balance = balance - amount;
        addTransaction(-amount, 0);
        cout << "  [OK] Withdrew $" << amount << "  New Balance: $" << balance << "\n";
    }

    void saveToFile(ofstream &f) {
        f << "ACCOUNT Current\n";
        f << accountId << " " << name << " " << balance << " " << permissions << "\n";
        f << "TRANSACTIONS\n";
        for (int i = 0; i < (int)transactions.size(); i++)
            f << transactions[i] << "\n";
        f << "END\n";
    }
};

// ----------------------------------------
// GLOBAL VECTOR TO STORE ALL ACCOUNTS
// ----------------------------------------
vector<Account*> accounts;

// find account by id
Account* findAccount(int id) {
    for (int i = 0; i < (int)accounts.size(); i++) {
        if (accounts[i]->getId() == id)
            return accounts[i];
    }
    return NULL;
}

// ----------------------------------------
// MENU FUNCTIONS
// ----------------------------------------
void createAccount() {
    int type, id;
    string name;
    double balance;

    cout << "\n  1 = Savings Account\n";
    cout << "  2 = Current Account\n";
    cout << "  Choose type: ";
    cin >> type;

    cout << "  Enter Account ID : "; cin >> id;
    cout << "  Enter Name       : "; cin >> name;
    cout << "  Enter Balance    : "; cin >> balance;

    // build permissions using bitwise OR
    unsigned int perm = 0;
    int choice;
    cout << "  Allow Withdraw? (1=Yes 0=No): "; cin >> choice;
    if (choice == 1) perm = perm | PERM_WITHDRAW;

    cout << "  Allow Deposit?  (1=Yes 0=No): "; cin >> choice;
    if (choice == 1) perm = perm | PERM_DEPOSIT;

    cout << "  Allow Transfer? (1=Yes 0=No): "; cin >> choice;
    if (choice == 1) perm = perm | PERM_TRANSFER;

    cout << "  VIP Account?    (1=Yes 0=No): "; cin >> choice;
    if (choice == 1) perm = perm | PERM_VIP;

    if (type == 1) {
        double rate;
        cout << "  Interest Rate (e.g. 0.05): "; cin >> rate;
        Account* acc = new SavingsAccount(id, name, balance, perm, rate);
        accounts.push_back(acc);
    } else {
        double overdraft;
        cout << "  Overdraft Limit: "; cin >> overdraft;
        Account* acc = new CurrentAccount(id, name, balance, perm, overdraft);
        accounts.push_back(acc);
    }

    cout << "  [OK] Account created! Permissions = " << perm << "\n";
}

void depositMoney() {
    int id; double amount;
    cout << "  Enter Account ID : "; cin >> id;
    cout << "  Enter Amount     : "; cin >> amount;
    Account* acc = findAccount(id);
    if (acc == NULL) { cout << "  [ERROR] Account not found!\n"; return; }
    acc->deposit(amount);
}

void withdrawMoney() {
    int id; double amount;
    cout << "  Enter Account ID : "; cin >> id;
    cout << "  Enter Amount     : "; cin >> amount;
    Account* acc = findAccount(id);
    if (acc == NULL) { cout << "  [ERROR] Account not found!\n"; return; }
    acc->withdraw(amount);
}

void showAccount() {
    int id;
    cout << "  Enter Account ID (0 = show all): "; cin >> id;
    if (id == 0) {
        for (int i = 0; i < (int)accounts.size(); i++)
            accounts[i]->showAccount();
    } else {
        Account* acc = findAccount(id);
        if (acc == NULL) cout << "  [ERROR] Account not found!\n";
        else acc->showAccount();
    }
}

void transferMoney() {
    int fromId, toId; double amount;
    cout << "  From Account ID : "; cin >> fromId;
    cout << "  To Account ID   : "; cin >> toId;
    cout << "  Amount          : "; cin >> amount;

    Account* from = findAccount(fromId);
    Account* to   = findAccount(toId);

    if (from == NULL || to == NULL) {
        cout << "  [ERROR] Account not found!\n"; return;
    }
    if (!from->hasPermission(PERM_TRANSFER)) {
        cout << "  [DENIED] No transfer permission!\n"; return;
    }

    from->withdraw(amount);
    to->deposit(amount);
    cout << "  [OK] Transfer complete!\n";
}

void saveToFile() {
    string filename;
    cout << "  Enter filename: "; cin >> filename;
    ofstream f(filename.c_str());
    if (!f) { cout << "  [ERROR] Cannot open file!\n"; return; }
    for (int i = 0; i < (int)accounts.size(); i++)
        accounts[i]->saveToFile(f);
    f.close();
    cout << "  [OK] Saved " << accounts.size() << " accounts to " << filename << "\n";
}

void loadFromFile() {
    string filename;
    cout << "  Enter filename: "; cin >> filename;
    ifstream f(filename.c_str());
    if (!f) { cout << "  [ERROR] File not found!\n"; return; }

    string line;
    int loaded = 0;

    while (getline(f, line)) {
        if (line == "ACCOUNT Savings" || line == "ACCOUNT Current") {
            string type = line.substr(8);
            int id; string name; double bal; unsigned int perm;
            f >> id >> name >> bal >> perm;
            f.ignore();

            Account* acc = NULL;
            if (type == "Savings")
                acc = new SavingsAccount(id, name, bal, perm, 0.03);
            else
                acc = new CurrentAccount(id, name, bal, perm, 500);

            accounts.push_back(acc);

            getline(f, line); // read "TRANSACTIONS"
            while (getline(f, line) && line != "END") {
                // transactions already loaded via balance
            }
            loaded++;
        }
    }
    f.close();
    cout << "  [OK] Loaded " << loaded << " accounts from " << filename << "\n";
}

// ----------------------------------------
// MAIN
// ----------------------------------------
int main() {
    int choice = 0;

    cout << "================================\n";
    cout << "   BANKING TRANSACTION SYSTEM   \n";
    cout << "================================\n";

    while (choice != 7) {
        cout << "\n--- MENU ---\n";
        cout << "1. Create Account\n";
        cout << "2. Deposit\n";
        cout << "3. Withdraw\n";
        cout << "4. Show Account\n";
        cout << "5. Save to File\n";
        cout << "6. Load from File\n";
        cout << "7. Exit\n";
        cout << "8. Transfer\n";
        cout << "Enter choice: ";
        cin >> choice;

        if      (choice == 1) createAccount();
        else if (choice == 2) depositMoney();
        else if (choice == 3) withdrawMoney();
        else if (choice == 4) showAccount();
        else if (choice == 5) saveToFile();
        else if (choice == 6) loadFromFile();
        else if (choice == 7) cout << "  Goodbye!\n";
        else if (choice == 8) transferMoney();
        else cout << "  Invalid choice!\n";
    }

    // free all dynamic memory
    for (int i = 0; i < (int)accounts.size(); i++)
        delete accounts[i];
    accounts.clear();

    return 0;
}
