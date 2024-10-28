#include <stdio.h>
#include <tgbot/tgbot.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Использование: %s <токен>\n", argv[0]);
        return 1;
    }
    TgBot::Bot bot(argv[1]);
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Привет, я Элечка, чем могу тебе помочь?");
    });
    try {
        printf("вошел в телеграмму как @%s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        printf("ошибка: %s\n", e.what());
    }
    return 0;
}