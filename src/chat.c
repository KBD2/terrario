#include <stdlib.h>
#include <string.h>

#include "chat.h"
#include "defs.h"

ChatMessage *lastMessage = NULL;

void sendMessage(char *message)
{
	ChatMessage *newMessage = malloc(sizeof(ChatMessage));

	*newMessage = (ChatMessage){
		.previous = lastMessage
	};
	newMessage->message = malloc(MESSAGE_MAX_LENGTH);
	strncpy(newMessage->message, message, MESSAGE_MAX_LENGTH);
	if(lastMessage != NULL) lastMessage->next = newMessage;
	lastMessage = newMessage;
}

void clearMessage(ChatMessage *message)
{
	if(message->previous != NULL) clearMessage(message->previous);
	if(message->next != NULL) message->next->previous = NULL;
	free(message->message);
	free(message);
}

void updateChat()
{
	ChatMessage *currMessage = lastMessage;

	while(currMessage != NULL)
	{
		currMessage->ticks++;
		if(currMessage->ticks == CHAT_MESSAGE_TICKS)
		{
			clearMessage(currMessage);
			if(currMessage == lastMessage) lastMessage = NULL;
			break;
		}
		else currMessage = currMessage->previous;
	}
}