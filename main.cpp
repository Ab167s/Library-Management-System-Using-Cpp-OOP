#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>

using namespace std;

// Forward declarations
class Book;
class User;
class Student;
class Librarian;
class Admin;

// Global constants
const string BOOKS_FILE = "books.txt";
const string USERS_FILE = "users.txt";
const string BORROWED_BOOKS_FILE = "borrowed_books.txt";
const int MAX_BORROW_DAYS = 7;
const double FINE_PER_DAY = 5.0; // fine 5 for each day of delay

// Utility functions for date handling
time_t getCurrentTime() {
    return time(nullptr);
}

int getDaysDifference(time_t start, time_t end) {
    return (end - start) / (60 * 60 * 24);
}

string timeToString(time_t time) {
    tm* timeInfo = localtime(&time);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d", timeInfo);
    return string(buffer);
}

time_t stringToTime(const string& timeStr) {
    tm timeInfo = {};
    istringstream ss(timeStr);
    ss >> get_time(&timeInfo, "%Y-%m-%d");
    return mktime(&timeInfo);
}

// Book class
class Book {
private:
    string title;
    string author;
    string status; // Available, Borrowed
    string isbn;

public:
    // Constructors
    Book() {}
    
    Book(const string& title, const string& author, const string& isbn) 
    : title(title), author(author), status("Available"), isbn(isbn) {}
    // Getters and setters
    string getTitle() const { return title; }
    string getAuthor() const { return author; }
    string getStatus() const { return status; }
    string getIsbn() const { return isbn; }
    
    void setTitle(const string& title) { this->title = title; }
    void setAuthor(const string& author) { this->author = author; }
    void setStatus(const string& status) { this->status = status; }
    void setIsbn(const string& isbn) { this->isbn = isbn; }
    
    // Methods
    bool checkAvailability() const { 
        return status == "Available"; 
    }
    
    void markAsBorrowed() { 
        status = "Borrowed"; 
    }
    
    void markAsReturned() { 
        status = "Available"; 
    }
    
    // Serialization for file storage
    string serialize() const {
        return isbn + "," + title + "," + author + "," + status;
    }
    
    // Deserialization from file storage
    static Book deserialize(const string& data) {
        Book book;
        stringstream ss(data);
        string item;
        vector<string> tokens;
        
        while (getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        
        if (tokens.size() >= 4) {
            book.isbn = tokens[0];
            book.title = tokens[1];
            book.author = tokens[2];
            book.status = tokens[3];
        }
        
        return book;
    }
    
    void display() const {
        cout << "ISBN: " << isbn << endl;
        cout << "Title: " << title << endl;
        cout << "Author: " << author << endl;
        cout << "Status: " << status << endl;
    }
};

// Forward declaration of the database manager
class DatabaseManager {
public:
    // Books management
    static vector<Book> getAllBooks();
    static bool saveBook(const Book& book);
    static bool updateBook(const Book& book);
    static bool removeBook(const string& isbn);
    static Book* findBookByIsbn(const string& isbn);
    
    // Borrowed books management
    static bool saveBorrowedBook(const string& studentId, const string& isbn, time_t borrowDate);
    static bool removeBorrowedBook(const string& studentId, const string& isbn);
    static vector<pair<string, time_t>> getBorrowedBooksByStudent(const string& studentId);
    static bool updateBorrowStatus(const Book& book);
    
    // Users management
    static bool saveUser(const User& user);
    static bool removeUser(const string& username);
    static User* findUserByUsername(const string& username);
    static vector<User*> getAllUsers();
};

// Base User class
class User {
protected:
    string username;
    string password;
    string role;

public:
    // Constructors
    User() {}
    
    User(const string& username, const string& password, const string& role)
        : username(username), password(password), role(role) {}
    
    // Virtual destructor for polymorphism
    virtual ~User() {}
    
    // Getters and setters
    string getUsername() const { return username; }
    string getPassword() const { return password; }
    string getRole() const { return role; }
    
    void setUsername(const string& username) { this->username = username; }
    void setPassword(const string& password) { this->password = password; }
    void setRole(const string& role) { this->role = role; }
    
    // Methods
    bool register_() const { 
    return DatabaseManager::saveUser(*this);
}
    
    bool login(const string& inputPassword) const {
        return password == inputPassword;
    }
    
    // Serialization for file storage
    virtual string serialize() const {
        return username + "," + password + "," + role;
    }
    
    // Deserialization is handled by specific user types
    
    // Display user info
    virtual void display() const {
        cout << "Username: " << username << endl;
        cout << "Role: " << role << endl;
    }
    
    // Factory method to create the right user type
    static User* createUser(const string& data);
};

// Student class
class Student : public User {
private:
    vector<pair<string, time_t>> borrowedBooks; // pair of ISBN and borrow date

public:
    // Constructors
    Student() { role = "student"; }
    
    Student(const string& username, const string& password)
        : User(username, password, "student") {
        // Load borrowed books
        borrowedBooks = DatabaseManager::getBorrowedBooksByStudent(username);
    }
    
    // Methods
    bool borrowBook(Book& book) {
        if (!book.checkAvailability()) {
            cout << "Book is not available for borrowing." << endl;
            return false;
        }
        
        book.markAsBorrowed();
        if (DatabaseManager::updateBook(book)) {
            time_t borrowDate = getCurrentTime();
            borrowedBooks.push_back(make_pair(book.getIsbn(), borrowDate));
            return DatabaseManager::saveBorrowedBook(username, book.getIsbn(), borrowDate);
        }
        
        return false;
    }
    
    bool returnBook(Book& book) {
        auto it = find_if(borrowedBooks.begin(), borrowedBooks.end(),
                         [&book](const pair<string, time_t>& b) {
                             return b.first == book.getIsbn();
                         });
        
        if (it == borrowedBooks.end()) {
            cout << "This book was not borrowed by this student." << endl;
            return false;
        }
        
        // Calculate fine if any
        time_t returnDate = getCurrentTime();
        int daysBorrowed = getDaysDifference(it->second, returnDate);
        
        if (daysBorrowed > MAX_BORROW_DAYS) {
            double fine = (daysBorrowed - MAX_BORROW_DAYS) * FINE_PER_DAY;
            cout << "Late return! Fine: " << fixed << setprecision(2) << fine << " USD" << endl;
        }
        
        book.markAsReturned();
        if (DatabaseManager::updateBook(book) && 
            DatabaseManager::removeBorrowedBook(username, book.getIsbn())) {
            borrowedBooks.erase(it);
            return true;
        }
        
        return false;
    }
    
    void viewBorrowedBooks() const {
        if (borrowedBooks.empty()) {
            cout << "No books currently borrowed." << endl;
            return;
        }
        
        cout << "Borrowed Books:" << endl;
        cout << "--------------------------------------" << endl;
        time_t currentTime = getCurrentTime();
        
        for (const auto& bookPair : borrowedBooks) {
            Book* book = DatabaseManager::findBookByIsbn(bookPair.first);
            if (book) {
                cout << "Title: " << book->getTitle() << endl;
                cout << "Author: " << book->getAuthor() << endl;
                cout << "Borrow Date: " << timeToString(bookPair.second) << endl;
                
                int daysBorrowed = getDaysDifference(bookPair.second, currentTime);
                cout << "Days Borrowed: " << daysBorrowed << endl;
                
                if (daysBorrowed > MAX_BORROW_DAYS) {
                    double fine = (daysBorrowed - MAX_BORROW_DAYS) * FINE_PER_DAY;
                    cout << "Current Fine: " << fixed << setprecision(2) << fine << " USD" << endl;
                } else {
                    cout << "Days Remaining: " << (MAX_BORROW_DAYS - daysBorrowed) << endl;
                }
                cout << "--------------------------------------" << endl;
                
                delete book;
            }
        }
    }
    
    void viewAvailableBooks() const {
        vector<Book> books = DatabaseManager::getAllBooks();
        cout << "Available Books:" << endl;
        cout << "--------------------------------------" << endl;
        
        for (const auto& book : books) {
            if (book.checkAvailability()) {
                cout << "ISBN: " << book.getIsbn() << endl;
                cout << "Title: " << book.getTitle() << endl;
                cout << "Author: " << book.getAuthor() << endl;
                cout << "--------------------------------------" << endl;
            }
        }
    }
    
    void searchBook(const string& query) const {
        vector<Book> books = DatabaseManager::getAllBooks();
        cout << "Search Results:" << endl;
        cout << "--------------------------------------" << endl;
        bool found = false;
        
        for (const auto& book : books) {
            if (book.getTitle().find(query) != string::npos || 
                book.getAuthor().find(query) != string::npos ||
                book.getIsbn().find(query) != string::npos) {
                cout << "ISBN: " << book.getIsbn() << endl;
                cout << "Title: " << book.getTitle() << endl;
                cout << "Author: " << book.getAuthor() << endl;
                cout << "Status: " << book.getStatus() << endl;
                cout << "--------------------------------------" << endl;
                found = true;
            }
        }
        
        if (!found) {
            cout << "No books found matching your query." << endl;
        }
    }
    
    // Serialization
    string serialize() const override {
        return User::serialize();
    }
    
    // Factory method implementation for deserialization
    static Student* deserialize(const string& data) {
        Student* student = new Student();
        stringstream ss(data);
        string item;
        vector<string> tokens;
        
        while (getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        
        if (tokens.size() >= 3) {
            student->username = tokens[0];
            student->password = tokens[1];
            student->role = tokens[2];
        }
        
        return student;
    }
    
    // Display student info
    void display() const override {
        User::display();
        viewBorrowedBooks();
    }
};

// Librarian class
class Librarian : public User {
private:
    vector<Book> managedBooks;

public:
    // Constructors
    Librarian() { role = "librarian"; }
    
    Librarian(const string& username, const string& password)
        : User(username, password, "librarian") {}
    
    // Methods
    bool addBook(const Book& book) {
        return DatabaseManager::saveBook(book);
    }
    
    bool updateBook(const Book& book) {
        return DatabaseManager::updateBook(book);
    }
    
    bool removeBook(const string& isbn) {
        return DatabaseManager::removeBook(isbn);
    }
    
    void viewInventory() const {
        vector<Book> books = DatabaseManager::getAllBooks();
        
        if (books.empty()) {
            cout << "No books in inventory." << endl;
            return;
        }
        
        cout << "Library Inventory:" << endl;
        cout << "--------------------------------------" << endl;
        
        for (const auto& book : books) {
            cout << "ISBN: " << book.getIsbn() << endl;
            cout << "Title: " << book.getTitle() << endl;
            cout << "Author: " << book.getAuthor() << endl;
            cout << "Status: " << book.getStatus() << endl;
            cout << "--------------------------------------" << endl;
        }
    }
    
    void viewAllMembers() const {
        vector<User*> users = DatabaseManager::getAllUsers();
        
        if (users.empty()) {
            cout << "No users registered in the system." << endl;
            return;
        }
        
        cout << "Library Members:" << endl;
        cout << "--------------------------------------" << endl;
        
        for (const auto& user : users) {
            if (user->getRole() == "student") {
                cout << "Username: " << user->getUsername() << endl;
                cout << "Role: " << user->getRole() << endl;
                cout << "--------------------------------------" << endl;
            }
            delete user;
        }
    }
    
    // Serialization
    string serialize() const override {
        return User::serialize();
    }
    
    // Factory method implementation for deserialization
    static Librarian* deserialize(const string& data) {
        Librarian* librarian = new Librarian();
        stringstream ss(data);
        string item;
        vector<string> tokens;
        
        while (getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        
        if (tokens.size() >= 3) {
            librarian->username = tokens[0];
            librarian->password = tokens[1];
            librarian->role = tokens[2];
        }
        
        return librarian;
    }
    
    // Display librarian info
    void display() const override {
        User::display();
    }
};

// Admin class
class Admin : public User {
public:
    // Constructors
    Admin() { role = "admin"; }
    
    Admin(const string& username, const string& password)
        : User(username, password, "admin") {}
    
    // Methods
    bool addStudent(const Student& student) {
    return student.register_(); 
    }
    
    bool addLibrarian(const Librarian& librarian) {
    return librarian.register_(); 
    }
    
    bool removeStudent(const string& username) {
        return DatabaseManager::removeUser(username);
    }
    
    bool removeLibrarian(const string& username) {
        return DatabaseManager::removeUser(username);
    }
    
    void viewOverdueReports() const {
        vector<Book> books = DatabaseManager::getAllBooks();
        time_t currentTime = getCurrentTime();
        bool hasOverdue = false;
        
        cout << "Overdue Books Report:" << endl;
        cout << "--------------------------------------" << endl;
        
        for (const auto& book : books) {
            if (book.getStatus() == "Borrowed") {
                // We need to get who borrowed this book and when
                vector<User*> users = DatabaseManager::getAllUsers();
                
                for (const auto& user : users) {
                    if (user->getRole() == "student") {
                        Student* student = dynamic_cast<Student*>(user);
                        if (student) {
                            auto borrowedBooks = DatabaseManager::getBorrowedBooksByStudent(student->getUsername());
                            
                            for (const auto& bookPair : borrowedBooks) {
                                if (bookPair.first == book.getIsbn()) {
                                    int daysBorrowed = getDaysDifference(bookPair.second, currentTime);
                                    
                                    if (daysBorrowed > MAX_BORROW_DAYS) {
                                        hasOverdue = true;
                                        double fine = (daysBorrowed - MAX_BORROW_DAYS) * FINE_PER_DAY;
                                        
                                        cout << "Book: " << book.getTitle() << endl;
                                        cout << "ISBN: " << book.getIsbn() << endl;
                                        cout << "Borrowed by: " << student->getUsername() << endl;
                                        cout << "Borrow Date: " << timeToString(bookPair.second) << endl;
                                        cout << "Days Overdue: " << (daysBorrowed - MAX_BORROW_DAYS) << endl;
                                        cout << "Fine: " << fixed << setprecision(2) << fine << " USD" << endl;
                                        cout << "--------------------------------------" << endl;
                                    }
                                }
                            }
                        }
                    }
                    delete user;
                }
            }
        }
        
        if (!hasOverdue) {
            cout << "No overdue books at this time." << endl;
        }
    }
    
    void viewLibraryStatus() const {
        vector<Book> books = DatabaseManager::getAllBooks();
        int totalBooks = books.size();
        int availableBooks = 0;
        int borrowedBooks = 0;
        
        for (const auto& book : books) {
            if (book.getStatus() == "Available") {
                availableBooks++;
            } else if (book.getStatus() == "Borrowed") {
                borrowedBooks++;
            }
        }
        
        vector<User*> users = DatabaseManager::getAllUsers();
        int totalUsers = 0;
        int students = 0;
        int librarians = 0;
        int admins = 0;
        
        for (const auto& user : users) {
            totalUsers++;
            if (user->getRole() == "student") {
                students++;
            } else if (user->getRole() == "librarian") {
                librarians++;
            } else if (user->getRole() == "admin") {
                admins++;
            }
            delete user;
        }
        
        cout << "Library Status Report:" << endl;
        cout << "--------------------------------------" << endl;
        cout << "Total Books: " << totalBooks << endl;
        cout << "Available Books: " << availableBooks << endl;
        cout << "Borrowed Books: " << borrowedBooks << endl;
        cout << "--------------------------------------" << endl;
        cout << "Total Users: " << totalUsers << endl;
        cout << "Students: " << students << endl;
        cout << "Librarians: " << librarians << endl;
        cout << "Admins: " << admins << endl;
        cout << "--------------------------------------" << endl;
    }
    
    // Serialization
    string serialize() const override {
        return User::serialize();
    }
    
    // Factory method implementation for deserialization
    static Admin* deserialize(const string& data) {
        Admin* admin = new Admin();
        stringstream ss(data);
        string item;
        vector<string> tokens;
        
        while (getline(ss, item, ',')) {
            tokens.push_back(item);
        }
        
        if (tokens.size() >= 3) {
            admin->username = tokens[0];
            admin->password = tokens[1];
            admin->role = tokens[2];
        }
        
        return admin;
    }
    
    // Display admin info
    void display() const override {
        User::display();
    }
};

// Factory method implementation for User
User* User::createUser(const string& data) {
    stringstream ss(data);
    string item;
    vector<string> tokens;
    
    while (getline(ss, item, ',')) {
        tokens.push_back(item);
    }
    
    if (tokens.size() >= 3) {
        string role = tokens[2];
        
        if (role == "student") {
            return Student::deserialize(data);
        } else if (role == "librarian") {
            return Librarian::deserialize(data);
        } else if (role == "admin") {
            return Admin::deserialize(data);
        }
    }
    
    return nullptr;
}

// DatabaseManager implementation
vector<Book> DatabaseManager::getAllBooks() {
    vector<Book> books;
    ifstream file(BOOKS_FILE);
    string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            if (!line.empty()) {
                books.push_back(Book::deserialize(line));
            }
        }
        file.close();
    }
    
    return books;
}

bool DatabaseManager::saveBook(const Book& book) {
    // Check if book already exists
    if (findBookByIsbn(book.getIsbn())) {
        return updateBook(book);
    }
    
    ofstream file(BOOKS_FILE, ios::app);
    if (file.is_open()) {
        file << book.serialize() << endl;
        file.close();
        return true;
    }
    
    return false;
}

bool DatabaseManager::updateBook(const Book& book) {
    vector<Book> books = getAllBooks();
    ofstream file(BOOKS_FILE);
    
    if (file.is_open()) {
        bool found = false;
        
        for (auto& b : books) {
            if (b.getIsbn() == book.getIsbn()) {
                file << book.serialize() << endl;
                found = true;
            } else {
                file << b.serialize() << endl;
            }
        }
        
        file.close();
        return found;
    }
    
    return false;
}

bool DatabaseManager::removeBook(const string& isbn) {
    vector<Book> books = getAllBooks();
    ofstream file(BOOKS_FILE);
    
    if (file.is_open()) {
        bool found = false;
        
        for (const auto& book : books) {
            if (book.getIsbn() != isbn) {
                file << book.serialize() << endl;
            } else {
                found = true;
            }
        }
        
        file.close();
        return found;
    }
    
    return false;
}

Book* DatabaseManager::findBookByIsbn(const string& isbn) {
    vector<Book> books = getAllBooks();
    
    for (const auto& book : books) {
        if (book.getIsbn() == isbn) {
            return new Book(book);
        }
    }
    
    return nullptr;
}

bool DatabaseManager::saveBorrowedBook(const string& studentId, const string& isbn, time_t borrowDate) {
    ofstream file(BORROWED_BOOKS_FILE, ios::app);
    
    if (file.is_open()) {
        file << studentId << "," << isbn << "," << timeToString(borrowDate) << endl;
        file.close();
        return true;
    }
    
    return false;
}

bool DatabaseManager::removeBorrowedBook(const string& studentId, const string& isbn) {
    ifstream inFile(BORROWED_BOOKS_FILE);
    string line;
    vector<string> lines;
    
    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            stringstream ss(line);
            string item;
            vector<string> tokens;
            
            while (getline(ss, item, ',')) {
                tokens.push_back(item);
            }
            
            if (tokens.size() >= 2 && !(tokens[0] == studentId && tokens[1] == isbn)) {
                lines.push_back(line);
            }
        }
        
        inFile.close();
        
        ofstream outFile(BORROWED_BOOKS_FILE);
        if (outFile.is_open()) {
            for (const auto& l : lines) {
                outFile << l << endl;
            }
            outFile.close();
            return true;
        }
    }
    
    return false;
}

vector<pair<string, time_t>> DatabaseManager::getBorrowedBooksByStudent(const string& studentId) {
    vector<pair<string, time_t>> borrowedBooks;
    ifstream file(BORROWED_BOOKS_FILE);
    string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            stringstream ss(line);
            string item;
            vector<string> tokens;
            
            while (getline(ss, item, ',')) {
                tokens.push_back(item);
            }
            
            if (tokens.size() >= 3 && tokens[0] == studentId) {
                borrowedBooks.push_back(make_pair(tokens[1], stringToTime(tokens[2])));
            }
        }
        
        file.close();
    }
    
    return borrowedBooks;
}

bool DatabaseManager::updateBorrowStatus(const Book& book) {
    return updateBook(book);
}

bool DatabaseManager::saveUser(const User& user) {
    // Check if user already exists
    if (findUserByUsername(user.getUsername())) {
        return false; // User already exists
    }
    
    ofstream file(USERS_FILE, ios::app);
    
    if (file.is_open()) {
        file << user.serialize() << endl;
        file.close();
        return true;
    }
    
    return false;
}

bool DatabaseManager::removeUser(const string& username) {
    vector<User*> users = getAllUsers();
    ofstream file(USERS_FILE);
    
    if (file.is_open()) {
        bool found = false;
        
        for (const auto& user : users) {
            if (user->getUsername() != username) {
                file << user->serialize() << endl;
            } else {
                found = true;
            }
            delete user;
        }
        
        file.close();
        return found;
    }
    
    for (auto& user : users) {
        delete user;
    }
    
    return false;
}

User* DatabaseManager::findUserByUsername(const string& username) {
    ifstream file(USERS_FILE);
    string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            stringstream ss(line);
            string item;
            vector<string> tokens;
            
            while (getline(ss, item, ',')) {
                tokens.push_back(item);
            }
            
            if (tokens.size() >= 3 && tokens[0] == username) {
                file.close();
                return User::createUser(line);
            }
        }
        
        file.close();
    }
    
    return nullptr;
}

vector<User*> DatabaseManager::getAllUsers() {
    vector<User*> users;
    ifstream file(USERS_FILE);
    string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            if (!line.empty()) {
                User* user = User::createUser(line);
                if (user) {
                    users.push_back(user);
                }
            }
        }
        
        file.close();
    }
    
    return users;
}

// LibrarySystem class to manage the application
class LibrarySystem {
private:
    User* currentUser;

    void logout() {
        if (currentUser) {
            delete currentUser;
            currentUser = nullptr;
            cout << "Logged out successfully." << endl;
        }
    }
    
    // Student menu and functions
    void studentMenu() {
        Student* student = dynamic_cast<Student*>(currentUser);
        if (!student) {
            cout << "Error accessing student menu. Logging out." << endl;
            logout();
            return;
        }
        
        int choice;
        while (true) {
            cout << "\n\n";
            cout << "=====================================\n";
            cout << "============ Student Menu ===========\n";
            cout << "=====================================\n";
            cout << "1. View Available Books\n";
            cout << "2. Borrow a Book\n";
            cout << "3. Return a Book\n";
            cout << "4. View My Borrowed Books\n";
            cout << "5. Search for a Book\n";
            cout << "6. Logout\n";
            cout << "0. Exit\n";
            cout << "-------------------------------------\n";
            cout << "Enter your choice: ";
            cin >> choice;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            string isbn, query;
            Book* book;
            
            switch (choice) {
                case 1:
                    student->viewAvailableBooks();
                    break;
                case 2:
                    cout << "Enter the ISBN of the book you want to borrow: ";
                    getline(cin, isbn);
                    book = DatabaseManager::findBookByIsbn(isbn);
                    if (book) {
                        if (student->borrowBook(*book)) {
                            cout << "\nBook borrowed successfully.\n";
                        } else {
                            cout << "\nFailed to borrow the book.\n";
                        }
                        delete book;
                    } else {
                        cout << "\nBook with ISBN " << isbn << " not found.\n";
                    }
                    break;
                case 3:
                    cout << "Enter the ISBN of the book you want to return: ";
                    getline(cin, isbn);
                    book = DatabaseManager::findBookByIsbn(isbn);
                    if (book) {
                        if (student->returnBook(*book)) {
                            cout << "\nBook returned successfully.\n";
                        } else {
                            cout << "\nFailed to return the book.\n";
                        }
                        delete book;
                    } else {
                        cout << "\nBook with ISBN " << isbn << " not found.\n";
                    }
                    break;
                case 4:
                    student->viewBorrowedBooks();
                    break;
                case 5:
                    cout << "Enter search query (title, author, or ISBN): ";
                    getline(cin, query);
                    student->searchBook(query);
                    break;
                case 6:
                    logout();
                    return;
                case 0:
                    cout << "Exiting the library system. Goodbye!" << endl;
                    exit(0);
                default:
                    cout << "\nInvalid choice. Please try again.\n";
            }
            
            cout << "\nPress Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    
    // Librarian menu and functions
    void librarianMenu() {
        Librarian* librarian = dynamic_cast<Librarian*>(currentUser);
        if (!librarian) {
            cout << "Error accessing librarian menu. Logging out." << endl;
            logout();
            return;
        }
        
        int choice;
        while (true) {
            cout << "\n\n";
            cout << "=====================================\n";
            cout << "========== Librarian Menu ===========\n";
            cout << "=====================================\n";
            cout << "1. Add a New Book\n";
            cout << "2. Update a Book\n";
            cout << "3. Remove a Book\n";
            cout << "4. View Inventory\n";
            cout << "5. View All Members\n";
            cout << "6. Logout\n";
            cout << "0. Exit\n";
            cout << "-------------------------------------\n";
            cout << "Enter your choice: ";
            cin >> choice;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            string isbn, title, author;
            Book* book;
            
            switch (choice) {
                case 1:
                    cout << "Enter ISBN: ";
                    getline(cin, isbn);
                    cout << "Enter Title: ";
                    getline(cin, title);
                    cout << "Enter Author: ";
                    getline(cin, author);
                    
                    book = DatabaseManager::findBookByIsbn(isbn);
                    if (book) {
                        cout << "\nA book with this ISBN already exists.\n";
                        delete book;
                    } else {
                        Book newBook(title, author, isbn);
                        if (librarian->addBook(newBook)) {
                            cout << "\nBook added successfully.\n";
                        } else {
                            cout << "\nFailed to add the book.\n";
                        }
                    }
                    break;
                case 2:
                    cout << "Enter ISBN of the book to update: ";
                    getline(cin, isbn);
                    
                    book = DatabaseManager::findBookByIsbn(isbn);
                    if (book) {
                        cout << "Current Title: " << book->getTitle() << endl;
                        cout << "Enter new Title (leave empty to keep current): ";
                        getline(cin, title);
                        
                        cout << "Current Author: " << book->getAuthor() << endl;
                        cout << "Enter new Author (leave empty to keep current): ";
                        getline(cin, author);
                        
                        if (!title.empty()) {
                            book->setTitle(title);
                        }
                        
                        if (!author.empty()) {
                            book->setAuthor(author);
                        }
                        
                        if (librarian->updateBook(*book)) {
                            cout << "\nBook updated successfully.\n";
                        } else {
                            cout << "\nFailed to update the book.\n";
                        }
                        
                        delete book;
                    } else {
                        cout << "\nBook with ISBN " << isbn << " not found.\n";
                    }
                    break;
                case 3:
                    cout << "Enter ISBN of the book to remove: ";
                    getline(cin, isbn);
                    
                    if (librarian->removeBook(isbn)) {
                        cout << "\nBook removed successfully.\n";
                    } else {
                        cout << "\nFailed to remove the book. Check if the ISBN is correct.\n";
                    }
                    break;
                case 4:
                    librarian->viewInventory();
                    break;
                case 5:
                    librarian->viewAllMembers();
                    break;
                case 6:
                    logout();
                    return;
                case 0:
                    cout << "Exiting the library system. Goodbye!" << endl;
                    exit(0);
                default:
                    cout << "\nInvalid choice. Please try again.\n";
            }
            
            cout << "\nPress Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
        
    // Admin menu and functions
    void adminMenu() {
        Admin* admin = dynamic_cast<Admin*>(currentUser);
        if (!admin) {
            cout << "Error accessing admin menu. Logging out." << endl;
            logout();
            return;
        }
        
        int choice;
        while (true) {
            cout << "\n\n";
            cout << "=====================================\n";
            cout << "============ Admin Menu =============\n";
            cout << "=====================================\n";
            cout << "1. Add a Student\n";
            cout << "2. Add a Librarian\n";
            cout << "3. Remove a Student\n";
            cout << "4. Remove a Librarian\n";
            cout << "5. View Overdue Reports\n";
            cout << "6. View Library Status\n";
            cout << "7. Logout\n";
            cout << "0. Exit\n";
            cout << "-------------------------------------\n";
            cout << "Enter your choice: ";
            cin >> choice;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            string username, password;
            User* user;
            
            switch (choice) {
                case 1:
                    cout << "Enter username for new student: ";
                    getline(cin, username);
                    
                    user = DatabaseManager::findUserByUsername(username);
                    if (user) {
                        cout << "\nUsername already exists.\n";
                        delete user;
                    } else {
                        cout << "Enter password for new student: ";
                        getline(cin, password);
                        
                        Student newStudent(username, password);
                        if (admin->addStudent(newStudent)) {
                            cout << "\nStudent added successfully.\n";
                        } else {
                            cout << "\nFailed to add student.\n";
                        }
                    }
                    break;
                case 2:
                    cout << "Enter username for new librarian: ";
                    getline(cin, username);
                    
                    user = DatabaseManager::findUserByUsername(username);
                    if (user) {
                        cout << "\nUsername already exists.\n";
                        delete user;
                    } else {
                        cout << "Enter password for new librarian: ";
                        getline(cin, password);
                        
                        Librarian newLibrarian(username, password);
                        if (admin->addLibrarian(newLibrarian)) {
                            cout << "\nLibrarian added successfully.\n";
                        } else {
                            cout << "\nFailed to add librarian.\n";
                        }
                    }
                    break;
                case 3:
                    cout << "Enter username of student to remove: ";
                    getline(cin, username);
                    
                    user = DatabaseManager::findUserByUsername(username);
                    if (user && user->getRole() == "student") {
                        delete user;
                        if (admin->removeStudent(username)) {
                            cout << "\nStudent removed successfully.\n";
                        } else {
                            cout << "\nFailed to remove student.\n";
                        }
                    } else {
                        cout << "\nStudent not found or user is not a student.\n";
                        if (user) delete user;
                    }
                    break;
                case 4:
                    cout << "Enter username of librarian to remove: ";
                    getline(cin, username);
                    
                    user = DatabaseManager::findUserByUsername(username);
                    if (user && user->getRole() == "librarian") {
                        delete user;
                        if (admin->removeLibrarian(username)) {
                            cout << "\nLibrarian removed successfully.\n";
                        } else {
                            cout << "\nFailed to remove librarian.\n";
                        }
                    } else {
                        cout << "\nLibrarian not found or user is not a librarian.\n";
                        if (user) delete user;
                    }
                    break;
                case 5:
                    admin->viewOverdueReports();
                    break;
                case 6:
                    admin->viewLibraryStatus();
                    break;
                case 7:
                    logout();
                    return;
                case 0:
                    cout << "Exiting the library system. Goodbye!" << endl;
                    exit(0);
                default:
                    cout << "\nInvalid choice. Please try again.\n";
            }
            
            cout << "\nPress Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
}

public:
    LibrarySystem() : currentUser(nullptr) {
        // Initialize files if they don't exist
        ofstream booksFile(BOOKS_FILE, ios::app);
        booksFile.close();
        
        ofstream usersFile(USERS_FILE, ios::app);
        usersFile.close();
        
        ofstream borrowedFile(BORROWED_BOOKS_FILE, ios::app);
        borrowedFile.close();
        
        // Create admin if not exists
        User* admin = DatabaseManager::findUserByUsername("admin");
        if (!admin) {
            Admin defaultAdmin("admin", "admin123");
            defaultAdmin.register_();
        } else {
            delete admin;
        }
    }
    
    ~LibrarySystem() {
        if (currentUser) {
            delete currentUser;
        }
    }
    
    void start() {
        int choice;
        
        do {
            if (!currentUser) {
                displayMainMenu();
                cin >> choice;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                
                switch (choice) {
                    case 1:
                        loginUser();
                        break;
                    case 2:
                        registerUser();
                        break;
                    case 0:
                        cout << "Exiting the library system. Goodbye!" << endl;
                        exit(0);
                    default:
                        cout << "\nInvalid choice. Please try again.\n";
                }
            } else {
                string role = currentUser->getRole();
                
                if (role == "student") {
                    studentMenu();
                } else if (role == "librarian") {
                    librarianMenu();
                } else if (role == "admin") {
                    adminMenu();
                } else {
                    cout << "Unknown role. Logging out." << endl;
                    logout();
                }
            }
        } while (true); 
    }
        
private:
     
    void displayMainMenu() {
    cout << "\n\n";
    cout << "=====================================\n";
    cout << "===== Library Management System ====\n";
    cout << "=====================================\n";
    cout << "1. Login\n";
    cout << "2. Register (Student)\n";
    cout << "0. Exit\n";
    cout << "-------------------------------------\n";
    cout << "Enter your choice: ";
    }


    void loginUser() {
        string username, password;
        
        cout << "Enter username: ";
        getline(cin, username);
        
        cout << "Enter password: ";
        getline(cin, password);
        
        User* user = DatabaseManager::findUserByUsername(username);
        
        if (user && user->login(password)) {
            cout << "Login successful! Welcome, " << username << "!" << endl;
            currentUser = user;
        } else {
            cout << "Invalid username or password." << endl;
            if (user) {
                delete user;
            }
        }
    }
    
    
    void registerUser() {
            string username, password;
            
            cout << "Enter username: ";
            getline(cin, username);
            
            // Check if username already exists
            User* existingUser = DatabaseManager::findUserByUsername(username);
            if (existingUser) {
                cout << "Username already exists. Please choose another one." << endl;
                delete existingUser;
                return;
            }
            
            cout << "Enter password: ";
            getline(cin, password);
            
            Student student(username, password);
            if (student.register_()) {
                cout << "Registration successful! You can now login." << endl;
            } else {
                cout << "Registration failed. Please try again." << endl;
            }
    }
        
    
    
};

// Main function
int main() {
    LibrarySystem library;
    library.start();
    return 0;
}