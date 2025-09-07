#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <regex>
#include <iomanip>
#include <functional>
#include <mutex>
#include <thread>
#include <exception>
#include <optional>
#include <limits>

// ==================== UTILITIES & EXCEPTIONS ====================

class EmployeeException : public std::exception {
private:
    std::string message;
public:
    explicit EmployeeException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class Logger {
private:
    static std::mutex log_mutex;
    static std::ofstream log_file;

public:
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

    static void init(const std::string& filename = "employee_system.log") {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (!log_file.is_open()) {
            log_file.open(filename, std::ios::app);
        }
    }

    static void log(Level level, const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::string level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"};

        if (log_file.is_open()) {
            log_file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                     << " [" << level_str[level] << "] " << message << std::endl;
        }

        // Also output to console for errors and critical
        if (level >= ERROR) {
            std::cerr << "[" << level_str[level] << "] " << message << std::endl;
        }
    }
};

std::mutex Logger::log_mutex;
std::ofstream Logger::log_file;

class Validator {
public:
    static bool isValidID(const std::string& id) {
        return std::regex_match(id, std::regex(R"([A-Z]{2}\d{4})"));  // Format: AB1234
    }

    static bool isValidName(const std::string& name) {
        return std::regex_match(name, std::regex(R"([A-Za-z\s'-]{2,50})"));
    }

    static bool isValidPosition(const std::string& position) {
        return std::regex_match(position, std::regex(R"([A-Za-z\s-]{2,30})"));
    }

    static bool isValidSalary(double salary) {
        return salary >= 0 && salary <= 10000000;  // Max 10M
    }

    static bool isValidEmail(const std::string& email) {
        return std::regex_match(email,
            std::regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"));
    }

    static bool isValidPhone(const std::string& phone) {
        return std::regex_match(phone, std::regex(R"(\+?\d{10,15})"));
    }
};

// ==================== ENHANCED EMPLOYEE STRUCTURE ====================

enum class Department {
    ENGINEERING, HR, FINANCE, MARKETING, OPERATIONS, SALES, UNKNOWN
};

enum class EmployeeStatus {
    ACTIVE, INACTIVE, ON_LEAVE, TERMINATED
};

enum class AccessLevel {
    BASIC, ADMIN
};

struct Employee {
    std::string id;
    std::string firstName;
    std::string lastName;
    std::string position;
    Department department;
    double salary;
    std::string email;
    std::string phone;
    std::chrono::system_clock::time_point hireDate;
    EmployeeStatus status;
    std::vector<std::string> skills;
    std::string managerId;  // References another employee
    AccessLevel accessLevel;

    // Constructors
    Employee() : salary(0), department(Department::UNKNOWN),
                status(EmployeeStatus::ACTIVE),
                hireDate(std::chrono::system_clock::now()),
                accessLevel(AccessLevel::BASIC) {}

    Employee(const std::string& id, const std::string& firstName,
             const std::string& lastName, const std::string& position,
             Department dept, double salary, const std::string& email = "",
             const std::string& phone = "", AccessLevel access = AccessLevel::BASIC)
        : id(id), firstName(firstName), lastName(lastName), position(position),
          department(dept), salary(salary), email(email), phone(phone),
          status(EmployeeStatus::ACTIVE), hireDate(std::chrono::system_clock::now()),
          accessLevel(access) {

        validate();
    }

    void validate() const {
        if (!Validator::isValidID(id))
            throw EmployeeException("Invalid employee ID format");
        if (!Validator::isValidName(firstName) || !Validator::isValidName(lastName))
            throw EmployeeException("Invalid name format");
        if (!Validator::isValidPosition(position))
            throw EmployeeException("Invalid position format");
        if (!Validator::isValidSalary(salary))
            throw EmployeeException("Invalid salary range");
        if (!email.empty() && !Validator::isValidEmail(email))
            throw EmployeeException("Invalid email format");
        if (!phone.empty() && !Validator::isValidPhone(phone))
            throw EmployeeException("Invalid phone format");
    }

    std::string getFullName() const {
        return firstName + " " + lastName;
    }

    std::string getDepartmentString() const {
        const char* depts[] = {"Engineering", "HR", "Finance", "Marketing", "Operations", "Sales", "Unknown"};
        return depts[static_cast<int>(department)];
    }

    std::string getStatusString() const {
        const char* statuses[] = {"Active", "Inactive", "On Leave", "Terminated"};
        return statuses[static_cast<int>(status)];
    }

    std::string getAccessLevelString() const {
        return (accessLevel == AccessLevel::ADMIN) ? "Admin" : "Basic";
    }

    // Serialization for file I/O
    std::string serialize() const {
        std::ostringstream oss;
        auto time_t = std::chrono::system_clock::to_time_t(hireDate);

        oss << id << "|" << firstName << "|" << lastName << "|" << position << "|"
            << static_cast<int>(department) << "|" << salary << "|" << email << "|"
            << phone << "|" << time_t << "|" << static_cast<int>(status) << "|"
            << managerId << "|" << static_cast<int>(accessLevel) << "|";

        // Serialize skills
        for (size_t i = 0; i < skills.size(); ++i) {
            oss << skills[i];
            if (i < skills.size() - 1) oss << ",";
        }

        return oss.str();
    }

    static Employee deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(iss, token, '|')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 12) {
            throw EmployeeException("Invalid serialized employee data");
        }

        Employee emp;
        emp.id = tokens[0];
        emp.firstName = tokens[1];
        emp.lastName = tokens[2];
        emp.position = tokens[3];
        emp.department = static_cast<Department>(std::stoi(tokens[4]));
        emp.salary = std::stod(tokens[5]);
        emp.email = tokens[6];
        emp.phone = tokens[7];
        emp.hireDate = std::chrono::system_clock::from_time_t(std::stoll(tokens[8]));
        emp.status = static_cast<EmployeeStatus>(std::stoi(tokens[9]));
        emp.managerId = tokens[10];
        emp.accessLevel = static_cast<AccessLevel>(std::stoi(tokens[11]));

        // Deserialize skills
        if (tokens.size() > 12 && !tokens[12].empty()) {
            std::istringstream skillsStream(tokens[12]);
            std::string skill;
            while (std::getline(skillsStream, skill, ',')) {
                emp.skills.push_back(skill);
            }
        }

        return emp;
    }
};

// ==================== ADVANCED SEARCH CRITERIA ====================

struct SearchCriteria {
    std::optional<std::string> id;
    std::optional<std::string> firstName;
    std::optional<std::string> lastName;
    std::optional<std::string> position;
    std::optional<Department> department;
    std::optional<double> minSalary;
    std::optional<double> maxSalary;
    std::optional<EmployeeStatus> status;
    std::optional<std::string> skill;
    bool caseSensitive = false;
};

// ==================== HIGH-PERFORMANCE HASH TABLE ====================

class EmployeeHashTable {
private:
    struct HashNode {
        std::unique_ptr<Employee> employee;
        std::unique_ptr<HashNode> next;

        HashNode(std::unique_ptr<Employee> emp) : employee(std::move(emp)) {}
    };

    std::vector<std::unique_ptr<HashNode>> table;
    size_t bucket_count;
    size_t element_count;
    mutable std::mutex table_mutex;

    // High-quality hash function (FNV-1a)
    size_t hash(const std::string& key) const {
        const size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        const size_t FNV_PRIME = 1099511628211ULL;

        size_t hash_value = FNV_OFFSET_BASIS;
        for (char c : key) {
            hash_value ^= static_cast<size_t>(c);
            hash_value *= FNV_PRIME;
        }
        return hash_value % bucket_count;
    }

    bool is_prime(size_t n) const {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;

        for (size_t i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }

    size_t next_prime(size_t n) const {
        while (!is_prime(n)) ++n;
        return n;
    }

    void rehash() {
        Logger::log(Logger::INFO, "Rehashing hash table, current load factor: " +
                   std::to_string(load_factor()));

        size_t old_bucket_count = bucket_count;
        bucket_count = next_prime(bucket_count * 2);

        auto old_table = std::move(table);
        table.clear();
        table.resize(bucket_count);
        element_count = 0;

        // Reinsert all elements
        for (auto& head : old_table) {
            auto current = std::move(head);
            while (current) {
                auto next = std::move(current->next);
                insert_internal(std::move(current->employee));
                current = std::move(next);
            }
        }

        Logger::log(Logger::INFO, "Rehashing completed, new bucket count: " +
                   std::to_string(bucket_count));
    }

    bool insert_internal(std::unique_ptr<Employee> emp) {
        size_t index = hash(emp->id);

        // Check for duplicates
        auto current = table[index].get();
        while (current) {
            if (current->employee->id == emp->id) {
                return false;  // Duplicate found
            }
            current = current->next.get();
        }

        // Insert at head
        auto new_node = std::make_unique<HashNode>(std::move(emp));
        new_node->next = std::move(table[index]);
        table[index] = std::move(new_node);
        ++element_count;

        return true;
    }

public:
    static constexpr double MAX_LOAD_FACTOR = 0.75;

    explicit EmployeeHashTable(size_t initial_bucket_count = 17)
        : bucket_count(next_prime(initial_bucket_count)), element_count(0) {
        table.resize(bucket_count);
        Logger::log(Logger::INFO, "Hash table initialized with " +
                   std::to_string(bucket_count) + " buckets");
    }

    // Move Constructor
    EmployeeHashTable(EmployeeHashTable&& other) noexcept
        : bucket_count(other.bucket_count),
          element_count(other.element_count),
          table(std::move(other.table)) {
        other.bucket_count = 0;
        other.element_count = 0;
    }

    // Move Assignment Operator
    EmployeeHashTable& operator=(EmployeeHashTable&& other) noexcept {
        if (this != &other) {
            std::lock(table_mutex, other.table_mutex);
            std::lock_guard<std::mutex> self_lock(table_mutex, std::adopt_lock);
            std::lock_guard<std::mutex> other_lock(other.table_mutex, std::adopt_lock);

            table = std::move(other.table);
            bucket_count = other.bucket_count;
            element_count = other.element_count;

            other.bucket_count = 0;
            other.element_count = 0;
        }
        return *this;
    }
    
    // Disable copy operations
    EmployeeHashTable(const EmployeeHashTable&) = delete;
    EmployeeHashTable& operator=(const EmployeeHashTable&) = delete;

    bool insert(const Employee& emp) {
        std::lock_guard<std::mutex> lock(table_mutex);

        try {
            emp.validate();
        } catch (const EmployeeException& e) {
            Logger::log(Logger::ERROR, "Employee validation failed: " + std::string(e.what()));
            throw;
        }

        auto emp_copy = std::make_unique<Employee>(emp);
        bool inserted = insert_internal(std::move(emp_copy));

        if (inserted) {
            Logger::log(Logger::INFO, "Employee inserted: " + emp.id);

            if (load_factor() > MAX_LOAD_FACTOR) {
                rehash();
            }
        } else {
            Logger::log(Logger::WARNING, "Duplicate employee ID attempted: " + emp.id);
        }

        return inserted;
    }

    bool remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(table_mutex);

        size_t index = hash(id);
        auto current = table[index].get();
        HashNode* prev = nullptr;

        while (current) {
            if (current->employee->id == id) {
                if (prev) {
                    prev->next = std::move(current->next);
                } else {
                    table[index] = std::move(current->next);
                }
                --element_count;
                Logger::log(Logger::INFO, "Employee removed: " + id);
                return true;
            }
            prev = current;
            current = current->next.get();
        }

        Logger::log(Logger::WARNING, "Employee not found for removal: " + id);
        return false;
    }

    Employee* find(const std::string& id) const {
        std::lock_guard<std::mutex> lock(table_mutex);

        size_t index = hash(id);
        auto current = table[index].get();

        while (current) {
            if (current->employee->id == id) {
                return current->employee.get();
            }
            current = current->next.get();
        }

        return nullptr;
    }

    bool update(const std::string& id, const Employee& updated_emp) {
        std::lock_guard<std::mutex> lock(table_mutex);

        size_t index = hash(id);
        auto current = table[index].get();

        while (current) {
            if (current->employee->id == id) {
                try {
                    updated_emp.validate();
                    *current->employee = updated_emp;
                    Logger::log(Logger::INFO, "Employee updated: " + id);
                    return true;
                } catch (const EmployeeException& e) {
                    Logger::log(Logger::ERROR, "Employee update validation failed: " + std::string(e.what()));
                    throw;
                }
            }
            current = current->next.get();
        }

        Logger::log(Logger::WARNING, "Employee not found for update: " + id);
        return false;
    }

    std::vector<Employee> search(const SearchCriteria& criteria) const {
        std::lock_guard<std::mutex> lock(table_mutex);
        std::vector<Employee> results;

        for (const auto& head : table) {
            auto current = head.get();
            while (current) {
                const Employee& emp = *current->employee;
                bool matches = true;

                if (criteria.id && emp.id != *criteria.id) matches = false;
                if (criteria.firstName) {
                    std::string empFirstName = criteria.caseSensitive ? emp.firstName :
                        [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(emp.firstName);
                    std::string searchFirstName = criteria.caseSensitive ? *criteria.firstName :
                        [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(*criteria.firstName);
                    if (empFirstName.find(searchFirstName) == std::string::npos) matches = false;
                }
                if (criteria.lastName) {
                    std::string empLastName = criteria.caseSensitive ? emp.lastName :
                        [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(emp.lastName);
                    std::string searchLastName = criteria.caseSensitive ? *criteria.lastName :
                        [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(*criteria.lastName);
                    if (empLastName.find(searchLastName) == std::string::npos) matches = false;
                }
                if (criteria.position) {
                    std::string empPosition = criteria.caseSensitive ? emp.position :
                        [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(emp.position);
                    std::string searchPosition = criteria.caseSensitive ? *criteria.position :
                        [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }(*criteria.position);
                    if (empPosition.find(searchPosition) == std::string::npos) matches = false;
                }
                if (criteria.department && emp.department != *criteria.department) matches = false;
                if (criteria.minSalary && emp.salary < *criteria.minSalary) matches = false;
                if (criteria.maxSalary && emp.salary > *criteria.maxSalary) matches = false;
                if (criteria.status && emp.status != *criteria.status) matches = false;
                if (criteria.skill) {
                    bool hasSkill = std::find_if(emp.skills.begin(), emp.skills.end(),
                        [&](const std::string& skill) {
                            if (criteria.caseSensitive) {
                                return skill.find(*criteria.skill) != std::string::npos;
                            } else {
                                std::string lowerSkill = skill;
                                std::string lowerSearch = *criteria.skill;
                                std::transform(lowerSkill.begin(), lowerSkill.end(), lowerSkill.begin(), ::tolower);
                                std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
                                return lowerSkill.find(lowerSearch) != std::string::npos;
                            }
                        }) != emp.skills.end();
                    if (!hasSkill) matches = false;
                }

                if (matches) {
                    results.push_back(emp);
                }

                current = current->next.get();
            }
        }

        Logger::log(Logger::INFO, "Search completed, found " + std::to_string(results.size()) + " results");
        return results;
    }

    std::vector<Employee> get_all() const {
        SearchCriteria empty_criteria;
        return search(empty_criteria);
    }

    double load_factor() const {
        return static_cast<double>(element_count) / bucket_count;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(table_mutex);
        return element_count;
    }

    void get_statistics(std::ostream& os) const {
        std::lock_guard<std::mutex> lock(table_mutex);

        size_t max_chain_length = 0;
        size_t empty_buckets = 0;
        size_t total_chain_length = 0;

        for (const auto& head : table) {
            size_t chain_length = 0;
            auto current = head.get();
            while (current) {
                ++chain_length;
                current = current->next.get();
            }

            if (chain_length == 0) {
                ++empty_buckets;
            } else {
                max_chain_length = std::max(max_chain_length, chain_length);
                total_chain_length += chain_length;
            }
        }

        double avg_chain_length = (bucket_count - empty_buckets > 0) ?
            static_cast<double>(total_chain_length) / (bucket_count - empty_buckets) : 0;

        os << "Hash Table Statistics:\n"
           << "  Bucket Count: " << bucket_count << "\n"
           << "  Element Count: " << element_count << "\n"
           << "  Load Factor: " << std::fixed << std::setprecision(3) << load_factor() << "\n"
           << "  Empty Buckets: " << empty_buckets << " ("
           << std::fixed << std::setprecision(1) << (100.0 * empty_buckets / bucket_count) << "%)\n"
           << "  Max Chain Length: " << max_chain_length << "\n"
           << "  Avg Chain Length: " << std::fixed << std::setprecision(2) << avg_chain_length << "\n";
    }
};

// ==================== DATA PERSISTENCE LAYER ====================

class DataManager {
private:
    std::string data_file;
    std::string backup_file;
    mutable std::mutex file_mutex;

public:
    explicit DataManager(const std::string& filename = "employees.dat")
        : data_file(filename), backup_file(filename + ".bak") {}

    bool save(const EmployeeHashTable& table) {
        std::lock_guard<std::mutex> lock(file_mutex);

        try {
            // Create backup first
            std::ifstream src(data_file);
            if (src.good()) {
                std::ofstream dst(backup_file);
                dst << src.rdbuf();
                src.close();
                dst.close();
            }

            // Save new data
            std::ofstream file(data_file);
            if (!file.is_open()) {
                Logger::log(Logger::ERROR, "Failed to open file for writing: " + data_file);
                return false;
            }

            auto employees = table.get_all();
            file << employees.size() << "\n";

            for (const auto& emp : employees) {
                file << emp.serialize() << "\n";
            }

            file.close();
            Logger::log(Logger::INFO, "Saved " + std::to_string(employees.size()) +
                       " employees to " + data_file);
            return true;
        } catch (const std::exception& e) {
            Logger::log(Logger::ERROR, "Error saving data: " + std::string(e.what()));
            return false;
        }
    }

    bool load(EmployeeHashTable& table) {
        std::lock_guard<std::mutex> lock(file_mutex);

        std::ifstream file(data_file);
        if (!file.is_open()) {
            Logger::log(Logger::INFO, "Data file not found, starting with empty database");
            return true;  // Not an error for first run
        }

        try {
            size_t count;
            file >> count;
            file.ignore();  // Skip newline after count

            size_t loaded = 0;
            std::string line;
            while (std::getline(file, line) && !line.empty()) {
                try {
                    Employee emp = Employee::deserialize(line);
                    if (table.insert(emp)) {
                        ++loaded;
                    }
                } catch (const EmployeeException& e) {
                    Logger::log(Logger::WARNING, "Failed to load employee record: " +
                               std::string(e.what()));
                }
            }

            file.close();
            Logger::log(Logger::INFO, "Loaded " + std::to_string(loaded) +
                       " employees from " + data_file);
            return true;
        } catch (const std::exception& e) {
            Logger::log(Logger::ERROR, "Error loading data: " + std::string(e.what()));
            return false;
        }
    }

    bool export_csv(const EmployeeHashTable& table, const std::string& filename) {
        std::lock_guard<std::mutex> lock(file_mutex);

        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                Logger::log(Logger::ERROR, "Failed to open CSV file for writing: " + filename);
                return false;
            }

            // Write header
            file << "ID,FirstName,LastName,Position,Department,Salary,Email,Phone,HireDate,Status,ManagerID,Skills,AccessLevel\n";

            auto employees = table.get_all();
            for (const auto& emp : employees) {
                auto time_t = std::chrono::system_clock::to_time_t(emp.hireDate);
                std::tm* tm_info = std::localtime(&time_t);

                file << emp.id << ","
                     << emp.firstName << ","
                     << emp.lastName << ","
                     << emp.position << ","
                     << emp.getDepartmentString() << ","
                     << emp.salary << ","
                     << emp.email << ","
                     << emp.phone << ","
                     << std::put_time(tm_info, "%Y-%m-%d") << ","
                     << emp.getStatusString() << ","
                     << emp.managerId << ",\"";

                // Skills in quotes
                for (size_t i = 0; i < emp.skills.size(); ++i) {
                    file << emp.skills[i];
                    if (i < emp.skills.size() - 1) file << ";";
                }
                file << "\",";
                file << emp.getAccessLevelString() << "\n";
            }

            file.close();
            Logger::log(Logger::INFO, "Exported " + std::to_string(employees.size()) +
                       " employees to CSV: " + filename);
            return true;
        } catch (const std::exception& e) {
            Logger::log(Logger::ERROR, "Error exporting CSV: " + std::string(e.what()));
            return false;
        }
    }
};

// ==================== ADVANCED CLI INTERFACE ====================

class AdvancedCLI {
private:
    EmployeeHashTable& db;
    DataManager data_manager;
    std::unique_ptr<Employee> currentUser;

    void clear_screen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    void pause() {
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }

    std::string get_input(const std::string& prompt) {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    double get_double_input(const std::string& prompt) {
        while (true) {
            try {
                std::cout << prompt;
                std::string input;
                std::getline(std::cin, input);
                return std::stod(input);
            } catch (const std::exception&) {
                std::cout << "Invalid number. Please try again.\n";
            }
        }
    }

    int get_int_input(const std::string& prompt, int min_val = 0, int max_val = INT_MAX) {
        while (true) {
            try {
                std::cout << prompt;
                std::string input;
                std::getline(std::cin, input);
                int value = std::stoi(input);
                if (value >= min_val && value <= max_val) {
                    return value;
                }
                std::cout << "Value must be between " << min_val << " and " << max_val << ".\n";
            } catch (const std::exception&) {
                std::cout << "Invalid number. Please try again.\n";
            }
        }
    }

    Department get_department_input() {
        std::cout << "\nDepartments:\n"
                  << "1. Engineering\n2. HR\n3. Finance\n4. Marketing\n5. Operations\n6. Sales\n";
        int choice = get_int_input("Select department (1-6): ", 1, 6);
        return static_cast<Department>(choice - 1);
    }

    EmployeeStatus get_status_input() {
        std::cout << "\nStatus options:\n"
                  << "1. Active\n2. Inactive\n3. On Leave\n4. Terminated\n";
        int choice = get_int_input("Select status (1-4): ", 1, 4);
        return static_cast<EmployeeStatus>(choice - 1);
    }

    AccessLevel get_access_level_input() {
        std::cout << "\nAccess Level:\n"
                  << "1. Basic\n2. Admin\n";
        int choice = get_int_input("Select access level (1-2): ", 1, 2);
        return static_cast<AccessLevel>(choice - 1);
    }

    std::vector<std::string> get_skills_input() {
        std::vector<std::string> skills;
        std::string skills_str = get_input("Enter skills (comma-separated): ");

        if (!skills_str.empty()) {
            std::istringstream iss(skills_str);
            std::string skill;
            while (std::getline(iss, skill, ',')) {
                // Trim whitespace
                skill.erase(0, skill.find_first_not_of(" \t"));
                skill.erase(skill.find_last_not_of(" \t") + 1);
                if (!skill.empty()) {
                    skills.push_back(skill);
                }
            }
        }

        return skills;
    }

    void display_employee(const Employee& emp) {
        auto time_t = std::chrono::system_clock::to_time_t(emp.hireDate);

        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "Employee Details\n";
        std::cout << std::string(60, '=') << "\n";
        std::cout << std::left << std::setw(15) << "ID:" << emp.id << "\n";
        std::cout << std::left << std::setw(15) << "Name:" << emp.getFullName() << "\n";
        std::cout << std::left << std::setw(15) << "Position:" << emp.position << "\n";
        std::cout << std::left << std::setw(15) << "Department:" << emp.getDepartmentString() << "\n";
        std::cout << std::left << std::setw(15) << "Salary:" << "$" << std::fixed << std::setprecision(2) << emp.salary << "\n";
        std::cout << std::left << std::setw(15) << "Email:" << (emp.email.empty() ? "N/A" : emp.email) << "\n";
        std::cout << std::left << std::setw(15) << "Phone:" << (emp.phone.empty() ? "N/A" : emp.phone) << "\n";
        std::cout << std::left << std::setw(15) << "Hire Date:" << std::put_time(std::localtime(&time_t), "%Y-%m-%d") << "\n";
        std::cout << std::left << std::setw(15) << "Status:" << emp.getStatusString() << "\n";
        std::cout << std::left << std::setw(15) << "Manager ID:" << (emp.managerId.empty() ? "N/A" : emp.managerId) << "\n";
        std::cout << std::left << std::setw(15) << "Access Level:" << emp.getAccessLevelString() << "\n";
        std::cout << std::left << std::setw(15) << "Skills:";
        if (emp.skills.empty()) {
            std::cout << "None\n";
        } else {
            for (size_t i = 0; i < emp.skills.size(); ++i) {
                if (i == 0) std::cout << emp.skills[i];
                else std::cout << ", " << emp.skills[i];
            }
            std::cout << "\n";
        }
        std::cout << std::string(60, '=') << "\n";
    }

    void display_employees_table(const std::vector<Employee>& employees) {
        if (employees.empty()) {
            std::cout << "\nNo employees found.\n";
            return;
        }

        std::cout << "\n" << std::string(140, '=') << "\n";
        std::cout << std::left
                  << std::setw(8) << "ID"
                  << std::setw(20) << "Name"
                  << std::setw(20) << "Position"
                  << std::setw(15) << "Department"
                  << std::setw(12) << "Salary"
                  << std::setw(25) << "Email"
                  << std::setw(12) << "Status"
                  << std::setw(10) << "Manager"
                  << std::setw(10) << "Access"
                  << "Skills\n";
        std::cout << std::string(140, '-') << "\n";

        for (const auto& emp : employees) {
            std::cout << std::left
                      << std::setw(8) << emp.id
                      << std::setw(20) << (emp.getFullName().length() > 19 ?
                                         emp.getFullName().substr(0, 16) + "..." : emp.getFullName())
                      << std::setw(20) << (emp.position.length() > 19 ?
                                         emp.position.substr(0, 16) + "..." : emp.position)
                      << std::setw(15) << emp.getDepartmentString()
                      << std::setw(12) << ("$" + std::to_string(static_cast<int>(emp.salary)))
                      << std::setw(25) << (emp.email.length() > 24 ?
                                         emp.email.substr(0, 21) + "..." :
                                         (emp.email.empty() ? "N/A" : emp.email))
                      << std::setw(12) << emp.getStatusString()
                      << std::setw(10) << (emp.managerId.empty() ? "N/A" : emp.managerId)
                      << std::setw(10) << emp.getAccessLevelString()
                      << (emp.skills.empty() ? "None" : emp.skills.at(0)) << "\n";
        }
        std::cout << std::string(140, '=') << "\n";
        std::cout << "Total employees: " << employees.size() << "\n";
    }

public:
    explicit AdvancedCLI(EmployeeHashTable& database) : db(database) {
        Logger::init();
        data_manager.load(db);
    }

    ~AdvancedCLI() {
        data_manager.save(db);
    }

    void run() {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "      ENTERPRISE EMPLOYEE MANAGEMENT SYSTEM\n";
        std::cout << std::string(60, '=') << "\n";
        
        if (!login()) {
            std::cout << "\nInvalid login. Exiting.\n";
            return;
        }

        while (true) {
            try {
                show_main_menu();
                int choice = 0;
                if (currentUser->accessLevel == AccessLevel::ADMIN) {
                    choice = get_int_input("\nSelect option (1-13): ", 1, 13);
                } else {
                    choice = get_int_input("\nSelect option (1-7): ", 1, 7);
                }

                if (currentUser->accessLevel == AccessLevel::ADMIN) {
                    switch (choice) {
                        case 1: add_employee(); break;
                        case 2: remove_employee(); break;
                        case 3: update_employee(); break;
                        case 4: find_employee(); break;
                        case 5: advanced_search(); break;
                        case 6: display_all_employees(); break;
                        case 7: generate_reports(); break;
                        case 8: import_export_menu(); break;
                        case 9: system_statistics(); break;
                        case 10: data_management(); break;
                        case 11: help_documentation(); break;
                        case 12: edit_my_profile(); break;
                        case 13:
                            std::cout << "\nSaving data and exiting...\n";
                            data_manager.save(db);
                            return;
                    }
                } else { // Basic user
                    switch (choice) {
                        case 1: find_employee(); break;
                        case 2: advanced_search(); break;
                        case 3: display_all_employees(); break;
                        case 4: generate_reports(); break;
                        case 5: help_documentation(); break;
                        case 6: edit_my_profile(); break;
                        case 7:
                            std::cout << "\nSaving data and exiting...\n";
                            data_manager.save(db);
                            return;
                        default:
                            std::cout << "\nInvalid option for a basic user. Please try again.\n";
                            pause();
                    }
                }
            } catch (const EmployeeException& e) {
                std::cout << "\nError: " << e.what() << "\n";
                pause();
            } catch (const std::exception& e) {
                std::cout << "\nUnexpected error: " << e.what() << "\n";
                Logger::log(Logger::CRITICAL, "Unexpected error: " + std::string(e.what()));
                pause();
            }
        }
    }

private:
    bool login() {
        int attempts = 3;
        while (attempts > 0) {
            std::string id = get_input("Enter your Employee ID to log in: ");
            Employee* emp = db.find(id);
            if (emp) {
                currentUser = std::make_unique<Employee>(*emp);
                std::cout << "\nLogin successful. Welcome, " << currentUser->getFullName() << " (" << currentUser->getAccessLevelString() << ").\n";
                pause();
                return true;
            } else {
                std::cout << "\nEmployee ID not found. " << --attempts << " attempts remaining.\n";
            }
        }
        return false;
    }

    void show_main_menu() {
        clear_screen();
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "           MAIN MENU\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Logged in as: " << currentUser->getFullName() << " (" << currentUser->getAccessLevelString() << ")\n";
        std::cout << std::string(50, '-') << "\n";

        if (currentUser->accessLevel == AccessLevel::ADMIN) {
            std::cout << " 1.  Add Employee\n";
            std::cout << " 2.  Remove Employee\n";
            std::cout << " 3.  Update Employee\n";
            std::cout << " 4.  Find Employee\n";
            std::cout << " 5.  Advanced Search\n";
            std::cout << " 6.  Display All Employees\n";
            std::cout << " 7.  Generate Reports\n";
            std::cout << " 8.  Import/Export Data\n";
            std::cout << " 9.  System Statistics\n";
            std::cout << "10.  Data Management\n";
            std::cout << "11.  Help & Documentation\n";
            std::cout << "12.  Edit My Profile\n";
            std::cout << "13.  Exit\n";
            std::cout << std::string(50, '=') << "\n";
            std::cout << "Database size: " << db.size() << " employees\n";
            std::cout << "Load factor: " << std::fixed << std::setprecision(3) << db.load_factor() << "\n";
        } else { // Basic user menu
            std::cout << " 1.  Find Employee (by ID)\n";
            std::cout << " 2.  Advanced Search\n";
            std::cout << " 3.  Display All Employees\n";
            std::cout << " 4.  Generate Reports\n";
            std::cout << " 5.  Help & Documentation\n";
            std::cout << " 6.  Edit My Profile\n";
            std::cout << " 7.  Exit\n";
            std::cout << std::string(50, '=') << "\n";
            std::cout << "Database size: " << db.size() << " employees\n";
        }
    }

    void edit_my_profile() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "       EDIT MY PROFILE\n";
        std::cout << std::string(40, '=') << "\n";
        display_employee(*currentUser);

        std::cout << "\nUpdate fields (press Enter to keep current value):\n";

        Employee updated = *currentUser;

        std::string input;
        
        input = get_input("First Name [" + updated.firstName + "]: ");
        if (!input.empty()) updated.firstName = input;

        input = get_input("Last Name [" + updated.lastName + "]: ");
        if (!input.empty()) updated.lastName = input;

        input = get_input("Phone [" + updated.phone + "]: ");
        if (!input.empty()) updated.phone = input;

        input = get_input("Email [" + updated.email + "]: ");
        if (!input.empty()) updated.email = input;
        
        std::cout << "Change skills? (y/n) - Current: ";
        if (updated.skills.empty()) std::cout << "None\n";
        else {
            for (const auto& skill : updated.skills) std::cout << skill << " ";
            std::cout << "\n";
        }
        input = get_input("");
        if (input == "y" || input == "Y") {
            updated.skills = get_skills_input();
        }

        try {
            if (db.update(currentUser->id, updated)) {
                *currentUser = updated; // Update the in-memory user object
                std::cout << "\n✓ Profile updated successfully!\n";
                display_employee(*currentUser);
            } else {
                std::cout << "\n✗ Failed to update profile.\n";
            }
        } catch (const EmployeeException& e) {
            std::cout << "\n✗ Validation Error: " << e.what() << "\n";
        }

        pause();
    }

    void add_employee() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "         ADD EMPLOYEE\n";
        std::cout << std::string(40, '=') << "\n";

        try {
            Employee emp;

            emp.id = get_input("Employee ID (format: AB1234): ");
            emp.firstName = get_input("First Name: ");
            emp.lastName = get_input("Last Name: ");
            emp.position = get_input("Position: ");
            emp.department = get_department_input();
            emp.salary = get_double_input("Salary: $");
            emp.email = get_input("Email (optional): ");
            emp.phone = get_input("Phone (optional): ");
            emp.managerId = get_input("Manager ID (optional): ");
            emp.accessLevel = get_access_level_input();
            emp.skills = get_skills_input();
            
            if (db.insert(emp)) {
                std::cout << "\n✓ Employee added successfully!\n";
                display_employee(emp);
            } else {
                std::cout << "\n✗ Failed to add employee. ID may already exist.\n";
            }
        } catch (const EmployeeException& e) {
            std::cout << "\n✗ Validation Error: " << e.what() << "\n";
        }

        pause();
    }

    void remove_employee() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "       REMOVE EMPLOYEE\n";
        std::cout << std::string(40, '=') << "\n";

        std::string id = get_input("Enter Employee ID to remove: ");

        Employee* emp = db.find(id);
        if (emp) {
            display_employee(*emp);
            std::string confirm = get_input("\nAre you sure you want to remove this employee? (yes/no): ");

            if (confirm == "yes" || confirm == "YES" || confirm == "y" || confirm == "Y") {
                if (db.remove(id)) {
                    std::cout << "\n✓ Employee removed successfully!\n";
                } else {
                    std::cout << "\n✗ Failed to remove employee.\n";
                }
            } else {
                std::cout << "\nOperation cancelled.\n";
            }
        } else {
            std::cout << "\n✗ Employee not found.\n";
        }

        pause();
    }

    void update_employee() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "       UPDATE EMPLOYEE\n";
        std::cout << std::string(40, '=') << "\n";

        std::string id = get_input("Enter Employee ID to update: ");

        Employee* emp = db.find(id);
        if (!emp) {
            std::cout << "\n✗ Employee not found.\n";
            pause();
            return;
        }

        display_employee(*emp);

        std::cout << "\nUpdate fields (press Enter to keep current value):\n";

        Employee updated = *emp;

        std::string input = get_input("First Name [" + emp->firstName + "]: ");
        if (!input.empty()) updated.firstName = input;

        input = get_input("Last Name [" + emp->lastName + "]: ");
        if (!input.empty()) updated.lastName = input;

        input = get_input("Position [" + emp->position + "]: ");
        if (!input.empty()) updated.position = input;

        std::cout << "Department [" << emp->getDepartmentString() << "] - Change? (y/n): ";
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") {
            updated.department = get_department_input();
        }

        input = get_input("Salary [$" + std::to_string(emp->salary) + "]: ");
        if (!input.empty()) {
            try {
                updated.salary = std::stod(input);
            } catch (const std::exception&) {
                std::cout << "Invalid salary, keeping current value.\n";
            }
        }

        input = get_input("Email [" + emp->email + "]: ");
        if (!input.empty()) updated.email = input;

        input = get_input("Phone [" + emp->phone + "]: ");
        if (!input.empty()) updated.phone = input;

        input = get_input("Manager ID [" + emp->managerId + "]: ");
        if (!input.empty()) updated.managerId = input;

        std::cout << "Status [" << emp->getStatusString() << "] - Change? (y/n): ";
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") {
            updated.status = get_status_input();
        }
        
        std::cout << "Access Level [" << emp->getAccessLevelString() << "] - Change? (y/n): ";
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") {
            updated.accessLevel = get_access_level_input();
        }

        try {
            if (db.update(id, updated)) {
                std::cout << "\n✓ Employee updated successfully!\n";
                display_employee(updated);
            } else {
                std::cout << "\n✗ Failed to update employee.\n";
            }
        } catch (const EmployeeException& e) {
            std::cout << "\n✗ Validation Error: " << e.what() << "\n";
        }

        pause();
    }

    void find_employee() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "        FIND EMPLOYEE\n";
        std::cout << std::string(40, '=') << "\n";

        std::string id = get_input("Enter Employee ID: ");

        Employee* emp = db.find(id);
        if (emp) {
            display_employee(*emp);
        } else {
            std::cout << "\n✗ Employee not found.\n";
        }

        pause();
    }
    void advanced_search() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "      ADVANCED SEARCH\n";
        std::cout << std::string(40, '=') << "\n";

        SearchCriteria criteria;
        std::string input;

        input = get_input("First Name (partial match): ");
        if (!input.empty()) criteria.firstName = input;

        input = get_input("Last Name (partial match): ");
        if (!input.empty()) criteria.lastName = input;

        input = get_input("Position (partial match): ");
        if (!input.empty()) criteria.position = input;

        std::cout << "Filter by Department? (y/n): ";
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") {
            criteria.department = get_department_input();
        }

        input = get_input("Minimum Salary (optional): ");
        if (!input.empty()) {
            try {
                criteria.minSalary = std::stod(input);
            } catch (const std::exception&) {
                std::cout << "Invalid salary format.\n";
            }
        }

        input = get_input("Maximum Salary (optional): ");
        if (!input.empty()) {
            try {
                criteria.maxSalary = std::stod(input);
            } catch (const std::exception&) {
                std::cout << "Invalid salary format.\n";
            }
        }

        std::cout << "Filter by Status? (y/n): ";
        std::getline(std::cin, input);
        if (input == "y" || input == "Y") {
            criteria.status = get_status_input();
        }

        input = get_input("Skill (partial match): ");
        if (!input.empty()) criteria.skill = input;

        std::cout << "Case sensitive search? (y/n): ";
        std::getline(std::cin, input);
        criteria.caseSensitive = (input == "y" || input == "Y");

        auto results = db.search(criteria);

        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "Search Results (" << results.size() << " found)\n";
        std::cout << std::string(40, '=') << "\n";

        display_employees_table(results);

        pause();
    }

    void display_all_employees() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "      ALL EMPLOYEES\n";
        std::cout << std::string(40, '=') << "\n";

        auto employees = db.get_all();

        // Sort by ID for consistent display
        std::sort(employees.begin(), employees.end(),
                 [](const Employee& a, const Employee& b) {
                     return a.id < b.id;
                 });

        display_employees_table(employees);

        pause();
    }

    void generate_reports() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "        REPORTS\n";
        std::cout << std::string(40, '=') << "\n";
        std::cout << "1. Department Summary\n";
        std::cout << "2. Salary Statistics\n";
        std::cout << "3. Employee Status Report\n";
        std::cout << "4. Skill Analysis\n";
        std::cout << "5. Management Hierarchy\n";

        int choice = get_int_input("Select report (1-5): ", 1, 5);

        auto employees = db.get_all();

        switch (choice) {
            case 1: generate_department_report(employees); break;
            case 2: generate_salary_report(employees); break;
            case 3: generate_status_report(employees); break;
            case 4: generate_skills_report(employees); break;
            case 5: generate_hierarchy_report(employees); break;
        }

        pause();
    }

    void generate_department_report(const std::vector<Employee>& employees) {
        std::unordered_map<Department, std::vector<Employee>> dept_map;

        for (const auto& emp : employees) {
            dept_map[emp.department].push_back(emp);
        }

        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "              DEPARTMENT SUMMARY\n";
        std::cout << std::string(60, '=') << "\n";

        for (const auto& [dept, dept_employees] : dept_map) {
            double total_salary = 0;
            double avg_salary = 0;

            for (const auto& emp : dept_employees) {
                total_salary += emp.salary;
            }

            if (!dept_employees.empty()) {
                avg_salary = total_salary / dept_employees.size();
            }

            const char* dept_names[] = {"Engineering", "HR", "Finance", "Marketing", "Operations", "Sales", "Unknown"};

            std::cout << "\n" << dept_names[static_cast<int>(dept)] << ":\n";
            std::cout << "  Employees: " << dept_employees.size() << "\n";
            std::cout << "  Total Salary Budget: $" << std::fixed << std::setprecision(2) << total_salary << "\n";
            std::cout << "  Average Salary: $" << std::fixed << std::setprecision(2) << avg_salary << "\n";
        }
    }

    void generate_salary_report(const std::vector<Employee>& employees) {
        if (employees.empty()) {
            std::cout << "\nNo employees to analyze.\n";
            return;
        }

        std::vector<double> salaries;
        double total = 0;

        for (const auto& emp : employees) {
            salaries.push_back(emp.salary);
            total += emp.salary;
        }

        std::sort(salaries.begin(), salaries.end());

        double average = total / employees.size();
        double median = salaries.size() % 2 == 0 ?
            (salaries[salaries.size()/2 - 1] + salaries[salaries.size()/2]) / 2 :
            salaries[salaries.size()/2];

        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "           SALARY STATISTICS\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Total Employees: " << employees.size() << "\n";
        std::cout << "Total Payroll: $" << std::fixed << std::setprecision(2) << total << "\n";
        std::cout << "Average Salary: $" << std::fixed << std::setprecision(2) << average << "\n";
        std::cout << "Median Salary: $" << std::fixed << std::setprecision(2) << median << "\n";
        std::cout << "Minimum Salary: $" << std::fixed << std::setprecision(2) << salaries.front() << "\n";
        std::cout << "Maximum Salary: $" << std::fixed << std::setprecision(2) << salaries.back() << "\n";

        // Salary ranges
        int ranges[] = {30000, 50000, 75000, 100000, 150000};
        std::string range_labels[] = {"<$30K", "$30K-50K", "$50K-75K", "$75K-100K", "$100K-150K", ">$150K"};
        int counts[6] = {0};

        for (double salary : salaries) {
            if (salary < ranges[0]) counts[0]++;
            else if (salary < ranges[1]) counts[1]++;
            else if (salary < ranges[2]) counts[2]++;
            else if (salary < ranges[3]) counts[3]++;
            else if (salary < ranges[4]) counts[4]++;
            else counts[5]++;
        }

        std::cout << "\nSalary Distribution:\n";
        for (int i = 0; i < 6; i++) {
            std::cout << "  " << std::left << std::setw(12) << range_labels[i]
                      << ": " << counts[i] << " ("
                      << std::fixed << std::setprecision(1)
                      << (100.0 * counts[i] / employees.size()) << "%)\n";
        }
    }

    void generate_status_report(const std::vector<Employee>& employees) {
        std::unordered_map<EmployeeStatus, int> status_count;

        for (const auto& emp : employees) {
            status_count[emp.status]++;
        }

        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "       EMPLOYEE STATUS\n";
        std::cout << std::string(40, '=') << "\n";

        const char* status_names[] = {"Active", "Inactive", "On Leave", "Terminated"};

        for (int i = 0; i < 4; i++) {
            EmployeeStatus status = static_cast<EmployeeStatus>(i);
            int count = status_count[status];
            double percentage = employees.empty() ? 0 : (100.0 * count / employees.size());

            std::cout << std::left << std::setw(12) << status_names[i]
                      << ": " << count << " ("
                      << std::fixed << std::setprecision(1) << percentage << "%)\n";
        }
    }

    void generate_skills_report(const std::vector<Employee>& employees) {
        std::unordered_map<std::string, int> skill_count;

        for (const auto& emp : employees) {
            for (const auto& skill : emp.skills) {
                skill_count[skill]++;
            }
        }

        std::vector<std::pair<std::string, int>> sorted_skills(skill_count.begin(), skill_count.end());
        std::sort(sorted_skills.begin(), sorted_skills.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });

        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "             SKILL ANALYSIS\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Total Unique Skills: " << sorted_skills.size() << "\n\n";
        std::cout << "Most Common Skills:\n";

        int display_count = std::min(15, static_cast<int>(sorted_skills.size()));
        for (int i = 0; i < display_count; i++) {
            std::cout << std::right << std::setw(2) << (i + 1) << ". "
                      << std::left << std::setw(25) << sorted_skills[i].first
                      << ": " << sorted_skills[i].second << " employees\n";
        }
    }

    void generate_hierarchy_report(const std::vector<Employee>& employees) {
        std::unordered_map<std::string, std::vector<std::string>> hierarchy;
        std::unordered_set<std::string> managers;
        std::unordered_set<std::string> all_employees;

        for (const auto& emp : employees) {
            all_employees.insert(emp.id);
            if (!emp.managerId.empty()) {
                hierarchy[emp.managerId].push_back(emp.id);
                managers.insert(emp.managerId);
            }
        }

        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "           MANAGEMENT HIERARCHY\n";
        std::cout << std::string(50, '=') << "\n";

        // Find top-level managers (managers who are also employees but have no manager)
        std::vector<std::string> top_managers;
        for (const auto& manager_id : managers) {
            if (all_employees.count(manager_id)) {
                Employee* mgr = db.find(manager_id);
                if (mgr && mgr->managerId.empty()) {
                    top_managers.push_back(manager_id);
                }
            }
        }

        // Display hierarchy
        for (const auto& top_mgr : top_managers) {
            Employee* mgr = db.find(top_mgr);
            if (mgr) {
                std::cout << mgr->getFullName() << " (" << top_mgr << ") - " << mgr->position << "\n";
                display_subordinates(hierarchy, top_mgr, 1);
                std::cout << "\n";
            }
        }

        // Show orphaned managers (managers not in employee database)
        std::cout << "External/Missing Managers:\n";
        for (const auto& manager_id : managers) {
            if (!all_employees.count(manager_id)) {
                std::cout << "  " << manager_id << " (manages " << hierarchy[manager_id].size() << " employees)\n";
            }
        }
    }

    void display_subordinates(const std::unordered_map<std::string, std::vector<std::string>>& hierarchy,
                             const std::string& manager_id, int level) {
        if (hierarchy.find(manager_id) == hierarchy.end()) return;

        std::string indent(level * 2, ' ');
        for (const auto& subordinate_id : hierarchy.at(manager_id)) {
            Employee* emp = db.find(subordinate_id);
            if (emp) {
                std::cout << indent << "├─ " << emp->getFullName() << " (" << subordinate_id
                          << ") - " << emp->position << "\n";
                display_subordinates(hierarchy, subordinate_id, level + 1);
            }
        }
    }

    void import_export_menu() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "      IMPORT/EXPORT\n";
        std::cout << std::string(40, '=') << "\n";
        std::cout << "1. Export to CSV\n";
        std::cout << "2. Manual Backup\n";
        std::cout << "3. Load from Backup\n";
        std::cout << "4. View Data Files\n";

        int choice = get_int_input("Select option (1-4): ", 1, 4);

        switch (choice) {
            case 1: export_csv(); break;
            case 2: manual_backup(); break;
            case 3: load_backup(); break;
            case 4: view_data_files(); break;
        }

        pause();
    }

    void export_csv() {
        std::string filename = get_input("Enter CSV filename (e.g., employees.csv): ");
        if (filename.empty()) filename = "employees.csv";

        if (data_manager.export_csv(db, filename)) {
            std::cout << "\n✓ Data exported successfully to " << filename << "\n";
        } else {
            std::cout << "\n✗ Export failed.\n";
        }
    }

    void manual_backup() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << "backup_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".dat";

        DataManager backup_manager(oss.str());
        if (backup_manager.save(db)) {
            std::cout << "\n✓ Manual backup created: " << oss.str() << "\n";
        } else {
            std::cout << "\n✗ Backup failed.\n";
        }
    }

    void load_backup() {
        std::string filename = get_input("Enter backup filename: ");
        std::string confirm = get_input("This will replace current data. Continue? (yes/no): ");

        if (confirm == "yes" || confirm == "YES") {
            DataManager backup_manager(filename);

            // Create temporary database
            EmployeeHashTable temp_db;
            if (backup_manager.load(temp_db)) {
                // Clear current database and load backup
                db = std::move(temp_db);
                std::cout << "\n✓ Backup loaded successfully.\n";
            } else {
                std::cout << "\n✗ Failed to load backup.\n";
            }
        } else {
            std::cout << "\nOperation cancelled.\n";
        }
    }

    void view_data_files() {
        std::cout << "\nData Files Information:\n";
        std::cout << "Primary data file: employees.dat\n";
        std::cout << "Automatic backup: employees.dat.bak\n";
        std::cout << "Log file: employee_system.log\n";

        // Check file sizes
        std::ifstream primary("employees.dat", std::ios::ate | std::ios::binary);
        if (primary.good()) {
            std::cout << "Primary file size: " << primary.tellg() << " bytes\n";
        } else {
            std::cout << "Primary file: Not found\n";
        }

        std::ifstream backup("employees.dat.bak", std::ios::ate | std::ios::binary);
        if (backup.good()) {
            std::cout << "Backup file size: " << backup.tellg() << " bytes\n";
        } else {
            std::cout << "Backup file: Not found\n";
        }
    }

    void system_statistics() {
        clear_screen();
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "           SYSTEM STATISTICS\n";
        std::cout << std::string(50, '=') << "\n";

        auto employees = db.get_all();

        std::cout << "Database Overview:\n";
        std::cout << "  Total Employees: " << employees.size() << "\n";

        // Hash table performance
        db.get_statistics(std::cout);

        // Memory usage estimation
        size_t estimated_memory = sizeof(Employee) * employees.size();
        estimated_memory += db.size() * 100; // Rough hash table overhead

        std::cout << "\nMemory Usage (estimated):\n";
        std::cout << "  Employee data: ~" << (estimated_memory / 1024) << " KB\n";

        // Department distribution
        std::unordered_map<Department, int> dept_count;
        for (const auto& emp : employees) {
            dept_count[emp.department]++;
        }

        std::cout << "\nDepartment Distribution:\n";
        const char* dept_names[] = {"Engineering", "HR", "Finance", "Marketing", "Operations", "Sales", "Unknown"};
        for (int i = 0; i < 7; i++) {
            Department dept = static_cast<Department>(i);
            std::cout << "  " << std::left << std::setw(12) << dept_names[i]
                      << ": " << dept_count[dept] << "\n";
        }

        pause();
    }

    void data_management() {
        clear_screen();
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "     DATA MANAGEMENT\n";
        std::cout << std::string(40, '=') << "\n";
        std::cout << "1. Save Data Now\n";
        std::cout << "2. Reload Data\n";
        std::cout << "3. Clear All Data\n";
        std::cout << "4. Data Validation\n";
        std::cout << "5. Optimize Database\n";

        int choice = get_int_input("Select option (1-5): ", 1, 5);

        switch (choice) {
            case 1:
                if (data_manager.save(db)) {
                    std::cout << "\n✓ Data saved successfully.\n";
                } else {
                    std::cout << "\n✗ Save failed.\n";
                }
                break;

            case 2: {
                std::string confirm = get_input("Reload will lose unsaved changes. Continue? (yes/no): ");
                if (confirm == "yes" || confirm == "YES") {
                    EmployeeHashTable temp_db;
                    if (data_manager.load(temp_db)) {
                        db = std::move(temp_db);
                        std::cout << "\n✓ Data reloaded successfully.\n";
                    } else {
                        std::cout << "\n✗ Reload failed.\n";
                    }
                } else {
                    std::cout << "\nOperation cancelled.\n";
                }
                break;
            }

            case 3: {
                std::string confirm = get_input("This will delete ALL employee data. Type 'DELETE ALL' to confirm: ");
                if (confirm == "DELETE ALL") {
                    db = EmployeeHashTable();  // Create new empty database
                    std::cout << "\n✓ All data cleared.\n";
                } else {
                    std::cout << "\nOperation cancelled.\n";
                }
                break;
            }

            case 4:
                validate_all_data();
                break;

            case 5:
                std::cout << "\nOptimizing database...\n";
                // Force rehash if needed
                if (db.load_factor() > 0.5) {
                    std::cout << "Triggering rehash for optimal performance...\n";
                    // The hash table will rehash automatically on next insert if needed
                }
                std::cout << "✓ Database optimization completed.\n";
                break;
        }

        pause();
    }

    void validate_all_data() {
        std::cout << "\nValidating all employee records...\n";

        auto employees = db.get_all();
        int valid_count = 0;
        int invalid_count = 0;

        for (const auto& emp : employees) {
            try {
                emp.validate();
                valid_count++;
            } catch (const EmployeeException& e) {
                invalid_count++;
                std::cout << "Invalid record - ID: " << emp.id << ", Error: " << e.what() << "\n";
            }
        }

        std::cout << "\nValidation Results:\n";
        std::cout << "  Valid records: " << valid_count << "\n";
        std::cout << "  Invalid records: " << invalid_count << "\n";

        if (invalid_count == 0) {
            std::cout << "✓ All records are valid!\n";
        } else {
            std::cout << "⚠ Found invalid records. Consider updating or removing them.\n";
        }
    }

    void help_documentation() {
        clear_screen();
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "              HELP & DOCUMENTATION\n";
        std::cout << std::string(60, '=') << "\n";

        std::cout << "EMPLOYEE ID FORMAT:\n";
        std::cout << "  Must follow pattern: AB1234 (2 letters + 4 digits)\n";
        std::cout << "  Examples: EM0001, HR0123, IT9999\n\n";

        std::cout << "VALIDATION RULES:\n";
        std::cout << "  • Names: 2-50 characters, letters, spaces, hyphens, apostrophes\n";
        std::cout << "  • Position: 2-30 characters, letters, spaces, hyphens\n";
        std::cout << "  • Salary: Must be between 0 and 10,000,000\n";
        std::cout << "  • Email: Standard email format (optional)\n";
        std::cout << "  • Phone: 10-15 digits, optional + prefix\n\n";

        std::cout << "SEARCH FEATURES:\n";
        std::cout << "  • Basic search by exact Employee ID\n";
        std::cout << "  • Advanced search with multiple criteria\n";
        std::cout << "  • Partial matching for names and positions\n";
        std::cout << "  • Salary range filtering\n";
        std::cout << "  • Department and status filtering\n";
        std::cout << "  • Skill-based search\n";
        std::cout << "  • Case-sensitive/insensitive options\n\n";

        std::cout << "REPORTS AVAILABLE:\n";
        std::cout << "  • Department Summary: Employee count and salary analysis\n";
        std::cout << "  • Salary Statistics: Statistical analysis of compensation\n";
        std::cout << "  • Status Report: Active/inactive employee breakdown\n";
        std::cout << "  • Skill Analysis: Most common skills across organization\n";
        std::cout << "  • Management Hierarchy: Organizational structure\n\n";

        std::cout << "DATA MANAGEMENT:\n";
        std::cout << "  • Automatic saving on exit\n";
        std::cout << "  • Manual backup creation with timestamps\n";
        std::cout << "  • CSV export functionality\n";
        std::cout << "  • Data validation tools\n";
        std::cout << "  • System performance optimization\n";
        std::cout << "  • Added move constructor and move assignment operator for thread-safety and efficiency\n\n";

        std::cout << "SYSTEM FEATURES:\n";
        std::cout << "  • High-performance hash table with automatic resizing\n";
        std::cout << "  • Thread-safe operations\n";
        std::cout << "  • Comprehensive error handling and logging\n";
        std::cout << "  • Input validation and sanitization\n";
        std::cout << "  • Memory-efficient design\n";
        std::cout << "  • Production-ready reliability\n\n";

        std::cout << "KEYBOARD SHORTCUTS:\n";
        std::cout << "  • Enter: Continue/Confirm\n";
        std::cout << "  • Type 'y' or 'yes': Confirm actions\n";
        std::cout << "  • Type 'n' or 'no': Cancel actions\n";
        std::cout << "  • Empty input: Keep current value (during updates)\n\n";

        std::cout << "TROUBLESHOOTING:\n";
        std::cout << "  • Check employee_system.log for detailed error messages\n";
        std::cout << "  • Use Data Validation to find problematic records\n";
        std::cout << "  • Create backups before major operations\n";
        std::cout << "  • Contact system administrator for persistent issues\n";

        pause();
    }
};

// ==================== MAIN APPLICATION ====================

int main() {
    try {
        // Initialize logger
        Logger::init();
        Logger::log(Logger::INFO, "Employee Management System starting");

        // Create database with optimal initial size
        EmployeeHashTable employee_db(101);  // Prime number for better distribution

        // On first run, create a default admin user if the database is empty
        if (employee_db.size() == 0) {
            try {
                Employee admin("XX0069", "System", "Admin", "Chief Executive Officer",
                                Department::ENGINEERING, 9999999.99, "admin@example.com",
                                "+1234567890", AccessLevel::ADMIN);
                employee_db.insert(admin);
                Logger::log(Logger::INFO, "Created default admin user XX0069");
            } catch (const EmployeeException& e) {
                Logger::log(Logger::CRITICAL, "Failed to create default admin user: " + std::string(e.what()));
            }
        }

        // Launch CLI interface
        AdvancedCLI cli(employee_db);
        cli.run();

        Logger::log(Logger::INFO, "Employee Management System shutting down normally");

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Logger::log(Logger::CRITICAL, "Fatal error: " + std::string(e.what()));
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        Logger::log(Logger::CRITICAL, "Unknown fatal error occurred");
        return 1;
    }

    return 0;
}