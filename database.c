#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

PGconn *conn;

void connect_db() {
    conn = PQconnectdb("host=localhost dbname=website user=postgres password=yourpassword port=5432");
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    printf("Connected to database.\n");
}

void disconnect_db() {
    PQfinish(conn);
    printf("Disconnected.\n");
}

void create_tables() {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS categories ("
        "   id SERIAL PRIMARY KEY,"
        "   name VARCHAR(100) UNIQUE NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS products ("
        "   id SERIAL PRIMARY KEY,"
        "   name VARCHAR(255) NOT NULL,"
        "   description TEXT,"
        "   price NUMERIC(10, 2) NOT NULL,"
        "   category_id INT REFERENCES categories(id) ON DELETE SET NULL,"
        "   created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS inventory ("
        "   id SERIAL PRIMARY KEY,"
        "   product_id INT REFERENCES products(id) ON DELETE CASCADE,"
        "   quantity INT NOT NULL DEFAULT 0,"
        "   last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Create tables failed: %s\n", PQerrorMessage(conn));
    } else {
        printf("Tables created.\n");
    }
    PQclear(res);
}

void insert_product(const char *name, const char *description, const char *price, const char *category_id) {
    const char *params[4] = { name, description, price, category_id };
    PGresult *res = PQexecParams(conn,
        "INSERT INTO products (name, description, price, category_id) VALUES ($1, $2, $3, $4)",
        4, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert product failed: %s\n", PQerrorMessage(conn));
    } else {
        printf("Product '%s' inserted.\n", name);
    }
    PQclear(res);
}

void insert_inventory(const char *product_id, const char *quantity) {
    const char *params[2] = { product_id, quantity };
    PGresult *res = PQexecParams(conn,
        "INSERT INTO inventory (product_id, quantity) VALUES ($1, $2)"
        " ON CONFLICT (product_id) DO UPDATE SET quantity = $2, last_updated = CURRENT_TIMESTAMP",
        2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Insert inventory failed: %s\n", PQerrorMessage(conn));
    } else {
        printf("Inventory updated for product ID %s.\n", product_id);
    }
    PQclear(res);
}

void get_all_products() {
    PGresult *res = PQexec(conn,
        "SELECT p.id, p.name, p.description, p.price, c.name AS category, i.quantity "
        "FROM products p "
        "LEFT JOIN categories c ON p.category_id = c.id "
        "LEFT JOIN inventory i ON p.id = i.product_id "
        "ORDER BY p.id");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    int rows = PQntuples(res);
    printf("\n%-5s %-30s %-10s %-15s %-10s\n", "ID", "Name", "Price", "Category", "Stock");
    printf("----------------------------------------------------------------------\n");
    for (int i = 0; i < rows; i++) {
        printf("%-5s %-30s %-10s %-15s %-10s\n",
            PQgetvalue(res, i, 0),
            PQgetvalue(res, i, 1),
            PQgetvalue(res, i, 3),
            PQgetvalue(res, i, 4),
            PQgetvalue(res, i, 5));
    }
    PQclear(res);
}

void delete_product(const char *product_id) {
    const char *params[1] = { product_id };
    PGresult *res = PQexecParams(conn,
        "DELETE FROM products WHERE id = $1",
        1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Delete failed: %s\n", PQerrorMessage(conn));
    } else {
        printf("Product ID %s deleted.\n", product_id);
    }
    PQclear(res);
}

int main() {
    connect_db();
    create_tables();

    insert_product("Wireless Mouse", "Ergonomic wireless mouse", "29.99", "1");
    insert_product("Mechanical Keyboard", "RGB mechanical keyboard", "79.99", "1");

    insert_inventory("1", "150");
    insert_inventory("2", "75");

    get_all_products();

    disconnect_db();
    return 0;
}
