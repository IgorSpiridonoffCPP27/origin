#include <iostream>
#include <pqxx/pqxx>

class ClientDB {
public:
    ClientDB(const std::string& conn_str) : conn(conn_str) {}

    void create_tables() {
        pqxx::work txn(conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS clients (
                id SERIAL PRIMARY KEY,
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                email TEXT UNIQUE NOT NULL
            );
        )");

        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS phones (
                id SERIAL PRIMARY KEY,
                client_id INTEGER REFERENCES clients(id) ON DELETE CASCADE,
                phone TEXT NOT NULL
            );
        )");

        txn.commit();
        std::cout << "Tables created.\n";
    }

    void add_client(const std::string& first, const std::string& last, const std::string& email) {
        pqxx::work txn(conn);
        txn.exec_params("INSERT INTO clients (first_name, last_name, email) VALUES ($1, $2, $3);", first, last, email);
        txn.commit();
        std::cout << "Client added.\n";
    }

    void add_phone(const std::string& email, const std::string& phone) {
        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT id FROM clients WHERE email = $1;", email);
        if (r.empty()) {
            std::cerr << "Client not found.\n";
            return;
        }
        int client_id = r[0][0].as<int>();
        txn.exec_params("INSERT INTO phones (client_id, phone) VALUES ($1, $2);", client_id, phone);
        txn.commit();
        std::cout << "Phone added.\n";
    }

    void update_client_email(const std::string& old_email, const std::string& new_email) {
        pqxx::work txn(conn);
        txn.exec_params("UPDATE clients SET email = $1 WHERE email = $2;", new_email, old_email);
        txn.commit();
        std::cout << "Email updated.\n";
    }

    void delete_phone(const std::string& email, const std::string& phone) {
        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params("SELECT id FROM clients WHERE email = $1;", email);
        if (r.empty()) {
            std::cerr << "Client not found.\n";
            return;
        }
        int client_id = r[0][0].as<int>();
        txn.exec_params("DELETE FROM phones WHERE client_id = $1 AND phone = $2;", client_id, phone);
        txn.commit();
        std::cout << "Phone deleted.\n";
    }

    void delete_client(const std::string& email) {
        pqxx::work txn(conn);
        txn.exec_params("DELETE FROM clients WHERE email = $1;", email);
        txn.commit();
        std::cout << "Client deleted.\n";
    }

    void find_client(const std::string& keyword) {
        pqxx::work txn(conn);
        pqxx::result r = txn.exec_params(R"(
            SELECT c.id, c.first_name, c.last_name, c.email, p.phone
            FROM clients c
            LEFT JOIN phones p ON c.id = p.client_id
            WHERE c.first_name ILIKE $1
               OR c.last_name ILIKE $1
               OR c.email ILIKE $1
               OR p.phone ILIKE $1;
        )", "%" + keyword + "%");

        for (auto row : r) {
            std::cout << "ID: " << row["id"].c_str()
                << ", Name: " << row["first_name"].c_str() << " " << row["last_name"].c_str()
                << ", Email: " << row["email"].c_str()
                << ", Phone: " << (row["phone"].is_null() ? "None" : row["phone"].c_str())
                << "\n";
        }

        if (r.empty()) {
            std::cout << "No client found.\n";
        }
    }

private:
    pqxx::connection conn;
};

int main() {
    try {
        ClientDB db("dbname=HomeworkTask3 user=postgres password=yourpass");

        db.create_tables();
        db.add_client("Ivan", "Petrov", "ivan.petrov@example.com");
        db.add_phone("ivan.petrov@example.com", "+71234567890");
        db.add_phone("ivan.petrov@example.com", "+79876543210");
        db.update_client_email("ivan.petrov@example.com", "ivan.new@example.com");
        db.find_client("ivan");
        db.delete_phone("ivan.new@example.com", "+71234567890");
        db.find_client("ivan");
        db.delete_client("ivan.new@example.com");
        db.find_client("ivan");

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
