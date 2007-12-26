#ifndef SDL_ConsoleCommands_H
#define SDL_ConsoleCommands_H



typedef struct CommandInfo_td
{
	void					(*CommandCallback)(char *Parameters);
	char					*CommandWord;
	struct CommandInfo_td	*NextCommand;
} CommandInfo;

#ifdef __cplusplus
extern "C" {
#endif

void CommandExecute(char *BackStrings);
void AddCommand(void (*CommandCallback)(char *Parameters),const char *CommandWord);
void TabCompletion(char* CommandLine, int *location);
void ListCommands();

#ifdef __cplusplus
};
#endif

#endif
