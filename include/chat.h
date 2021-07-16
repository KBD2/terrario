#pragma once

/*
----- CHAT -----

The chat display.
*/

struct ChatMessageStruct {
    unsigned short ticks;
    char *message;
    struct ChatMessageStruct *previous;
    struct ChatMessageStruct *next;
};

typedef struct ChatMessageStruct ChatMessage;

extern ChatMessage *lastMessage;

/* sendMessage
Sends a message to the chat.

message: The message to send.
*/
void sendMessage(char *message);

/* updateChat
Updates chat, should be called every engine tick.
*/
void updateChat();