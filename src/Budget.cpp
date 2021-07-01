#include <tgbot/tgbot.h>
#include <cstdlib>
#include "Budget.h"

Budget::Budget() {
    int db_open = 0;
    db_open = sqlite3_open("db.db", &db);

    if (db_open) {
        std::cerr << "Error open DB " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }
    loadCats();
}

Budget::~Budget() {
    sqlite3_close(db);
}

std::vector<std::string> Budget::getCats() const {
    return catNames;
}

uint Budget::storeItem(const std::string &message, int32_t user_id) {
    std::stringstream ss(message);
    std::string cat;
    double amount;
    ss >> cat >> amount;

    std::string sql = "insert into items (cat_id, amount, created_on, orig, created_by) values "
                      "(?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(
            db,
            sql.c_str(),
            sql.length(),
            &stmt,
            nullptr);

    std::string catId;
    if (cats.count(cat) != 0) {
        catId = cats[cat];
    } else {
        catId = cats["other"];
    }

    sqlite3_bind_text(
            stmt,
            1,
            catId.c_str(),
            catId.length(),
            SQLITE_STATIC);

    sqlite3_bind_double(stmt, 2, amount);

    sqlite3_bind_int(stmt, 3, time(nullptr));

    sqlite3_bind_text(
            stmt,
            4,
            message.c_str(),
            message.length(),
            SQLITE_STATIC);

    sqlite3_bind_int(stmt, 5, user_id);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    sql = "select last_insert_rowid()";
    sqlite3_prepare_v2(
            db,
            sql.c_str(),
            sql.length(),
            &stmt,
            nullptr);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        uint item_id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return item_id;
    }

    return 0;
}

int Budget::deleteItem(uint id, int32_t user_id) {
    std::string sql = "update items set archived=1 where id=? and created_by=?";

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(
            db,
            sql.c_str(),
            sql.length(),
            &stmt,
            nullptr);

    sqlite3_bind_int(
            stmt,
            1,
            id);

    sqlite3_bind_int(
            stmt,
            2,
            user_id);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return sqlite3_changes(db);
}

std::unordered_map<std::string, double> Budget::today() const {
    return report("day");
}

std::unordered_map<std::string, double> Budget::month() const {
    return report("month");
}

void Budget::loadCats() {
    sqlite3_stmt *stmt;
    std::string sql = "select id, name, aliases from cats";

    if (sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "db error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string cat_id;
        std::string name;
        std::vector<std::string> aliases;

        for (int col = 0; col < sqlite3_column_count(stmt); col++) {

            std::string col_name = sqlite3_column_name(stmt, col);

            if ("id" == col_name) {
                cat_id = std::string(reinterpret_cast<const char *>(
                                             sqlite3_column_text(stmt, col)
                                     ));
            }
            if ("name" == col_name) {
                name = std::string(reinterpret_cast<const char *>(
                                           sqlite3_column_text(stmt, col)
                                   ));
            }
            if ("aliases" == col_name) {
                std::stringstream ss(reinterpret_cast<const char *>(
                                             sqlite3_column_text(stmt, col)
                                     ));

                while (ss.good()) {
                    std::string substr;
                    getline(ss, substr, ',');
                    aliases.push_back(substr);
                }
                break;
            }
        }

        cats[cat_id] = cat_id;
        cats[name] = cat_id;
        for (const std::string &alias : aliases) {
            cats[alias] = cat_id;
        }
        catNames.push_back(name);
    }
    sqlite3_finalize(stmt);
}

std::unordered_map<std::string, double> Budget::report(const std::string &period) const {
    std::string sql = "select c.name, sum(i.amount) as total "
                      "from items i "
                      "join cats c on c.id = i.cat_id "
                      "where i.created_on >= strftime('%s', datetime('now', 'start of " + period + "')) "
                      "and i.archived=0 "
                      "group by i.cat_id";

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(
            db,
            sql.c_str(),
            sql.length(),
            &stmt,
            nullptr);

    std::unordered_map<std::string, double> res;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string cat_id;
        double amount;

        cat_id = std::string(reinterpret_cast<const char *>(
                                     sqlite3_column_text(stmt, 0)
                             ));

        amount = sqlite3_column_double(stmt, 1);

        res[cat_id] = amount;
    }

    sqlite3_finalize(stmt);

    return res;
}
