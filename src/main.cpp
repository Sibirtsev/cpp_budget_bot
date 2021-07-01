#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <unordered_map>

#include <tgbot/tgbot.h>
#include "Budget.h"

std::string buildReportMessage(const std::unordered_map<std::string, double>& report)
{
    if (report.empty()) {
        return "Расходы не найдены.\nНужно срочно что-то купить :)";
    }

    double total = 0;
    std::stringstream ss;
    ss << "Уже потрачено:\n";
    for (const auto& item : report) {
        ss << " - " << item.first << " : " << item.second << "\n";
        total += item.second;
    }
    ss << "\n---------------\nИтого: " << total;

    return ss.str();
}

int main()
{
    std::string token(getenv("TOKEN"));

    std::vector<int32_t> senders;
    std::stringstream ss(getenv("SENDERS"));
    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, ',');
        senders.push_back(stoi(substr));
    }

    Budget budget;

    TgBot::Bot bot(token);
    bot.getEvents().onCommand("start", [&bot](const TgBot::Message::Ptr& message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
    });

    bot.getEvents().onCommand("cats", [&bot, &budget](const TgBot::Message::Ptr& message) {
        std::vector<std::string> cats = budget.getCats();
        std::stringstream ss;
        for (const std::string& cat : cats) {
            ss << " - " << cat << "\n";
        }
        bot.getApi().sendMessage(message->chat->id, "Доступные категории:\n" + ss.str());
    });

    bot.getEvents().onAnyMessage([&bot, &senders, &budget](const TgBot::Message::Ptr& message) {
        if (find(senders.begin(), senders.end(), message->from->id) == senders.end()) {
            printf("Access Denied! User %d\n", message->from->id);
            bot.getApi().sendMessage(message->chat->id, "Access Denied!");
            return;
        }

        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        if (StringTools::startsWith(message->text, "/cats")) {
            return;
        }
        if (StringTools::startsWith(message->text, "/today")) {
            std::unordered_map<std::string, double> report = budget.today();
            bot.getApi().sendMessage(message->chat->id, buildReportMessage(report));
            return;
        }
        if (StringTools::startsWith(message->text, "/month")) {
            std::unordered_map<std::string, double> report = budget.month();
            bot.getApi().sendMessage(message->chat->id, buildReportMessage(report));
            return;
        }
        if (StringTools::startsWith(message->text, "/del")) {
            uint item_id = stoi(message->text.substr(4, message->text.length() - 1));
            int deleted = budget.deleteItem(item_id, message->from->id);
            bot.getApi().sendMessage(message->chat->id, deleted ? "Удалено" : "Расход не найден");
            return;
        }

        uint new_item_id = budget.storeItem(message->text, message->from->id);
        std::stringstream answer;
        answer << "Новая трата добавлена\nЧтобы её удалить /del" << new_item_id << "\n\n";
        answer << "Посмотреть отчёт за сегодня /today \n";
        answer << "За текущий месяц /month \n";
        bot.getApi().sendMessage(message->chat->id, answer.str());
    });

    signal(SIGINT, [](int s) {
        printf("SIGINT got\n");
        exit(0);
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (std::exception& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}