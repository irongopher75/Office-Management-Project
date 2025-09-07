Employee Management System

Project Description

Since the completion of the Data Structures and Algorithms Course and my two years of exposure of the C++ programming language i wanted to demonstrate the real life working of my skills in managing an organization for both the administration and the users. I have made it as a CLI(Command Line Interface) Project. The Program lets you save the information you feed in it and can access it even after exiting the program as it has a custom logger that enables its functionality. Other than that one should first log in from the admin and add employee to gain access of the basic interface.

This project is a robust, command-line interface (CLI) application built with C++ that serves as a proof of concept for my skills in data structures, algorithms, and systems programming. Drawing from my two years of experience with C++ and formal training in a Data Structures and Algorithms course, this system demonstrates a practical, real-world application for managing organizational data.

The core of the system is designed around a high-performance hash table, chosen for its efficiency in handling a high volume of employee records. This data structure allows for near-instantaneous O(1) average-case time complexity for operations like adding, searching for, and removing employees. To ensure long-term stability and performance, the hash table includes an intelligent rehashing mechanism that automatically resizes the underlying array when the load factor exceeds a certain threshold.

To make the system both user-friendly and secure, I've implemented a role-based access control (RBAC) system. This design ensures that administrators have full control over the database, while standard employees can only view and update their own information. A special "admin" user with ID "XX0069" is created on the first run of the program, providing a secure entry point for managing the organization's data.

All data is managed through a persistent storage layer, meaning information is saved to a file (employees.dat) and can be accessed even after the program has been closed and restarted. This functionality is supported by a custom-built logging system that tracks all major operations, providing a detailed audit trail for administrators. In the event of an error, the system also creates a backup of the data to prevent data loss.

In addition to basic management functions, the application includes a comprehensive suite of features to provide value to both administrators and employees:

Advanced Search: Users can perform detailed searches based on multiple criteria, including name, position, department, and salary range.

Detailed Reports: The system can generate various reports, such as a departmental summary of employee counts and salaries, a statistical analysis of company-wide compensation, and a complete organizational hierarchy.

Data Integrity: All data entered into the system is rigorously validated using regular expressions to ensure it meets predefined standards for fields like ID, name, email, and phone number.

This project goes beyond a simple academic exercise by simulating the challenges of real-world enterprise software, including data persistence, security, and performance.

Dependencies

This project relies exclusively on the C++ Standard Library and does not require any external third-party dependencies. To compile and run this program, you will need a C++ compiler that supports the C++17 standard or newer.

If you don't already have a C++ compiler installed on your system, follow the instructions below for your specific operating system.

How to Install a C++ Compiler

macOS

macOS uses Clang as its default compiler, which is part of the Xcode Command Line Tools.

Open Terminal.

You can find it in Applications/Utilities.

Install Xcode Command Line Tools by running the following command:

Bash
xcode-select --install
This will prompt a pop-up window to guide you through the installation process. Once completed, you will have the clang++ compiler ready for use.

Windows

For Windows, the most common way to get a C++ compiler is to install the MinGW-w64 toolchain.

Download the MinGW-w64 installer.

You can download it from the official website or a trusted source like SourceForge.

Run the installer.

During installation, ensure you select the x86_64 architecture.

Add MinGW to your system's PATH.

This step is crucial for running the compiler commands from any terminal.

Search for "Environment Variables" in the Windows search bar and open it.

In the System Properties window, click Environment Variables.

Under "System variables," find and select Path, then click Edit.

Click New and paste the path to your MinGW bin directory (e.g., C:\MinGW\bin or C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin).

Click OK on all windows to save the changes.

Verify the installation.

Close and reopen your command prompt or PowerShell, and then run:

Bash
g++ --version
A version number output confirms the installation was successful.

Compilation

Bash
# Production build (optimized) for macOS/Linux
clang++ -std=c++17 -O3 -Wall -Wextra -pthread -o employee_system main.cpp

# Production build (optimized) for Windows
g++ -std=c++17 -O3 -Wall -Wextra -pthread -o employee_system.exe main.cpp
Running the Application

Bash
# Run the system on macOS/Linux
./employee_system

# Run the system on Windows
.\employee_system.exe

# First time setup
# Default admin credentials: ID = XX0069
⚙️ System Architecture

The application is built on a layered architecture to ensure clear separation of concerns, scalability, and maintainability.

1. Command Line Interface (CLI)
This serves as the central entry point for all user operations. It handles user input and routes requests to the appropriate modules for processing, providing clear feedback and an intuitive user experience.

2. Data Layer
This layer is responsible for the in-memory storage and management of employee records.

EmployeeHashTable: A custom hash table implementation that provides efficient storage and retrieval of employee data.

HashNodes: The core components of the hash table, they store individual employee records and use chaining to handle data collisions.

3. Persistence Layer
This layer manages the interaction between the program's in-memory data and the file system.

DataManager: A manager class that orchestrates the loading and saving of data.

File I/O: Ensures reliable and persistent storage of employee records, so data is not lost when the program exits.

4. Monitoring Layer
This layer is crucial for tracking and auditing the system's behavior.

Logger: A utility that tracks all major operations, providing a detailed record for debugging and accountability.

Log Files: The physical files where historical system records are maintained.

Each layer is designed to be independent, allowing for modular development and easier debugging. The CLI acts as the coordinator, interacting with these specialized layers to provide the full functionality of the employee management system.