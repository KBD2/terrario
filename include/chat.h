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

/* clearMessage
Clears the given message and any previous messages.

message: Pointer to the message to clear
*/
void clearMessage(ChatMessage *message);

/* updateChat
Updates chat, should be called every engine tick.
*/
void updateChat();