/*  SDL_Console.c
 *  Written By: Garrett Banuk <mongoose@wpi.edu>
 *  This is free, just be sure to give me credit when using it
 *  in any of your programs.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL.h>
#include "SDL_Console.h"
#include "SDL_ConsoleCommands.h"


/* Linked list of all the commands. */
static CommandInfo	*Commands = NULL;


/* executes the command passed in from the string */
void CommandExecute(char *BackStrings )
{
	char		Command[CHARS_PER_LINE];
	CommandInfo	*CurrentCommand = Commands;
	
	/* Get the command out of the string */
	if(EOF == sscanf(BackStrings, "%s", Command))
		return;

	NewLineConsole();

	/* Find the command nd execute the function associated with it */
	while(CurrentCommand)
	{
		if(0 == strcmp(Command, CurrentCommand->CommandWord))
		{
			CurrentCommand->CommandCallback(BackStrings+strlen(Command));
			return;
		}
		CurrentCommand = CurrentCommand->NextCommand;
	}

	/* Command wasn't found */
	ConOut("Bad Command");

}

/* Use this to add commands.
 * Pass in a pointer to a funtion that will take a string which contains the 
 * arguments passed to the command. Second parameter is the command to execute
 * on.
 */
void AddCommand( void(*CommandCallback)(char *Parameters),const char *CommandWord)
{
	CommandInfo	**CurrentCommand = &Commands;


	while(*CurrentCommand)
		CurrentCommand = &((*CurrentCommand)->NextCommand);
	

	/* Add a command to the list */
	*CurrentCommand = (CommandInfo*)malloc(sizeof(CommandInfo));
	(*CurrentCommand)->CommandCallback = CommandCallback;
	(*CurrentCommand)->CommandWord = (char*)malloc(strlen(CommandWord)+1);
	strcpy((*CurrentCommand)->CommandWord, CommandWord);
	(*CurrentCommand)->NextCommand = NULL;

	
}

/* tab completes commands, not perfect finds the first matching command */
void TabCompletion(char *CommandLine, int *location )
{
	CommandInfo	*CurrentCommand = Commands;


	while(CurrentCommand)
	{
		if(0 == strncmp(CommandLine, CurrentCommand->CommandWord, strlen(CommandLine)))
		{
			strcpy(CommandLine, CurrentCommand->CommandWord);
			CommandLine[strlen(CurrentCommand->CommandWord)] = ' ';
			*location = strlen(CurrentCommand->CommandWord)+1;
			UpdateConsole();
			return;
		}
		CurrentCommand = CurrentCommand->NextCommand;
	}
}


/* Lists all the commands to be used in the console */
void ListCommands()
{
	CommandInfo	*CurrentCommand = Commands;
	
	ConOut(" ");
	while(CurrentCommand)
	{
		ConOut("%s", CurrentCommand->CommandWord);
		CurrentCommand = CurrentCommand->NextCommand;
	}
	
}
