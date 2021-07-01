#ifndef BUDGET_BOT_BUDGET_H
#define BUDGET_BOT_BUDGET_H

#include <string>
#include <vector>
#include <unordered_map>
#include <sqlite3.h>

class Budget {
public:
    Budget();

    ~Budget();

    std::vector<std::string> getCats() const;

    uint storeItem(const std::string &message, int32_t user_id);

    int deleteItem(uint id, int32_t user_id);

    std::unordered_map<std::string, double> today() const;

    std::unordered_map<std::string, double> month() const;

private:
    void loadCats();

    std::unordered_map<std::string, double> report(const std::string &period) const;

    std::unordered_map<std::string, std::string> cats;
    std::vector<std::string> catNames;
    sqlite3 *db{};
};

#endif //BUDGET_BOT_BUDGET_H
