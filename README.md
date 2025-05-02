# 📚 Library Management System (C++ / OOP)

A **console-based application** for managing a library system, developed using Object-Oriented Programming (OOP) principles in **C++**. It features multi-role user access and **file-based persistent storage**.

---

## ✨ Key Features

### 👥 User Roles
| Role        | Capabilities                                                                 |
|-------------|------------------------------------------------------------------------------|
| **Student** | Borrow/return books • Search catalog • View borrowed books & fines          |
| **Librarian** | Add/update/delete books • Manage book inventory • View member records     |
| **Admin**   | Create/delete user accounts • Generate overdue reports • View system stats  |

### ⚙️ Technical Highlights
- 📁 **File-based storage**: `books.txt`, `users.txt`, `borrowed_books.txt`
- ⏳ **Fine calculation**: $5/day for books returned **after 7 days**
- 🔐 **Role-based access control**: Admin > Librarian > Student

---

## 🚀 Getting Started

### 📦 Prerequisites
- A **C++ compiler** (e.g., `G++`, `MinGW`, or Visual Studio)

### 🛠️ Installation & Running
```bash
# Compile the program
g++ main.cpp -o library

# Run the program
./library         # On Linux / macOS
.\library.exe     # On Windows

### 🔐 Default Login Credentials

| Role  | Username | Password   |
|-------|----------|------------|
| Admin | admin    | admin123   |

---

### 📂 Data File Structure

| File                 | Format                                  | Example Entry                             |
|----------------------|------------------------------------------|-------------------------------------------|
| `books.txt`          | ISBN, Title, Author, Status              | `100,Clean Code,R. Martin,Available`      |
| `users.txt`          | Username, Password, Role                 | `john,pass123,student`                    |
| `borrowed_books.txt` | Username, ISBN, Borrow Date (YYYY-MM-DD)| `john,100,2023-11-20`                     |
