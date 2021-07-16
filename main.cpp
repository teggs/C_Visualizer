#include "frame.h"

#if	defined(__linux__)
#include <sys/wait.h>
#endif

//  THE APPLICATION'S "main" FUNCTIONS ARE DEFINED HERE:
//
//  - COMMAND-LINE ARGUMENTS ARE CHECKED
//  - THE GUI IS CREATED
//  - THE SINGLE C SOURCE FILE IS COMPILED (IF NECESSARY)
//  - gdb IS STARTED AS AN EXTERNAL PROCESS

class MyApp : public wxApp
{
public:
    bool OnInit(void)				// execution commences here
    {
	char **cargv	= (char **)argv;
	argv0		= (argv0 = strrchr(cargv[0],'/')) ? argv0+1 : cargv[0];

//  CHECK FOR THE CORRECT NUMBER OF COMMAND-LINE ARGUMENTS
	if(argc != 2) {
	    fprintf(stderr, "Usage: %s file.c\n", argv0);
	    exit(EXIT_FAILURE);
	}

//  ENSURE THAT A C SOURCE FILE WAS GIVEN
	char *source	= cargv[1];

	if(!has_suffix(source, ".c")) {
	    fprintf(stderr, "%s: %s does not have .c suffix\n", argv0, source);
	    exit(EXIT_FAILURE);
	}
// Get File
	FILE* fp = fopen(source,"r");

	if (fp == NULL){
	    perror(source);
	    exit(EXIT_FAILURE);
	}
//  COMPILE THE C SOURCE FILE, PRODUCING THE EXECUTABLE TO BE DEBUGGED
	char *exe	= strdup(source);
	char *dot	= strrchr(exe, '.');
	*dot		= '\0';

	if(CompileSource(source, exe) != 0) {
	    fprintf(stderr, "%s: cannot compile %s\n", argv0, source);
	    exit(EXIT_FAILURE);
	}

//  gdb IS STARTED AS AN EXTERNAL PROCESS
	gdb_pid	= StartGDB(exe);
	if(gdb_pid < 0) {
	    fprintf(stderr, "%s: cannot execute '%s %s'\n", argv0, GDB_NAME, exe);
	    exit(EXIT_FAILURE);
	}

//  CREATE THE GUI (IMPLEMENTED IN gui.cpp)
	wxString title = wxString::Format("%s %s - %s",
				APPNAME, APPVERSION, source);
	new MyFrame(title, fd_togdb, fd_fromgdb, fp);
	fclose(fp);

//  REPORT OUR PROGRESS TO GUI's OUTPUT PANEL
//	outpanel->AddLine( wxString::Format("compiled %s %s %s",
//		source, rarrow, exe));
//	outpanel->AddLine( wxString::Format("started '%s %s' with pid=%i",
//		GDB_NAME, exe, gdb_pid));
//	outpanel->AddLine("");

	wxApp::SetExitOnFrameDelete(true);
	SetAppName(APPNAME);
	return true;
    }

private:
//  WHEN GUI IS EXITING, TERMINATE THE gdb PROCESS
    int OnExit(void)
    {
	if(gdb_pid > 0) {
	    close(fd_togdb);			// close communication channels
	    close(fd_fromgdb);

	    kill(gdb_pid, SIGINT);		// send signals to terminate
	    kill(gdb_pid, SIGTERM);
	    kill(gdb_pid, SIGKILL);
	}
	return 0;
    }

//  DETERMINE IF THE PROVIDED FILENAME HAS THE REQUIRED SUFFIX
    bool has_suffix(const char *filenm, const char *suffix)
    {
	if(filenm == NULL || suffix == NULL)
	    return false;

	size_t lenfilenm    = strlen(filenm);
	size_t lensuffix    = strlen(suffix);

	if(lensuffix >  lenfilenm)
	    return false;

	return (strcmp(filenm + lenfilenm - lensuffix, suffix) == 0);   // magic!
    }

//  COMPILE THE SINGLE C SOURCE FILE (IF NECESSARY) TO PRODUCE THE EXECUTABLE
    int CompileSource(char *source, char *exe)
    {
	pid_t	pid;
	int	status = -1;

	if(access(source, R_OK) != 0) {		// can we read the source file?
	    perror(source);
	    return 1;
	}

	struct stat	stat_source, stat_exe;

	if(stat(exe, &stat_exe) == 0) {		// does the executable exist?
	    stat(source, &stat_source);
	    if(stat_exe.st_mtime > stat_source.st_mtime)	// exe is newer
		return 0;
	}

	switch (pid = fork()) {			// fork a new process
	case -1 :
	    perror("fork");
	    return 1;
	    break;

	case 0 : {				// new process runs C compiler
	    char *av[] = {			// arguments for the compiler
		    (char *)"cc",
		    (char *)"-std=c99",
		    (char *)"-g",
		    (char *)"-o",
		    exe,
		    source,
		    NULL
	    };
	    execvp(av[0], av);
	    perror(av[0]);
	    exit(EXIT_FAILURE);
	    break;
	}
	default :
	    while(wait(&status) != pid)		// wait for 'cc' to exit
		;
	    break;
	}
	return WEXITSTATUS(status); 		// return compiler's exit status
    }

//  START gdb AS AN EXTERNAL PROCESS
    int StartGDB(char *exe)
    {
	int	to[2], from[2];
	pid_t	pid;

//  WE SETUP TWO COMMUNICATION PIPES, ONE TO SEND COMMANDS TO gdb, AND
//  THE OTHER TO RECEIVE THE OUTPUT FROM gdb
	if(pipe(to) != 0 || pipe(from) != 0) {	// able to create 2 pipes?
	    perror("pipe");
	    return -1;
	}

	switch (pid = fork()) {			// fork a new process
	case -1 :
	    perror("fork");
	    break;

	case 0 : {				// child process, gdb
//  CONFIGURE DESCRIPTORS SO THAT gdb READS stdin FROM THE GUI PROCESS
	    close(0);
	    dup2(to[0], 0);			// stdin from pipe
	    close(to[0]);
	    close(to[1]);

//  CONFIGURE DESCRIPTORS SO THAT gdb WRITES stdout+stderr TO THE GUI PROCESS
	    close(1);
	    close(2);
	    dup2(from[1], 1);			// stdout to pipe
	    dup2(from[1], 2);			// stderr to pipe
	    close(from[0]);
	    close(from[1]);

	    char *av[] = {			// arguments for gdb
		    (char *)GDB_NAME,
//		    (char *)"-quiet",
		    exe,
		    NULL
	    };
	    execv(GDB_PATH, av);
	    perror(av[0]);
	    exit(EXIT_FAILURE);
	    break;
	}
	default :				// parent GUI process
//  CONFIGURE DESCRIPTORS SO THAT THE GUI WRITES ITS stdout TO gdb
	    fd_togdb	= to[1];
	    close(to[0]);

//  CONFIGURE DESCRIPTORS SO THAT THE GUI READS ITS stdin FROM gdb's OUTPUT
	    fd_fromgdb	= from[0];
	    int value	= fcntl(fd_fromgdb, F_GETFL, NULL);
	    value       |= O_NONBLOCK;
	    fcntl(fd_fromgdb, F_SETFL, value);
	    close(from[1]);
	    break;
	}
	return pid;				// return pid of gdb
    }

    char	*argv0;
    int		gdb_pid;
    int		fd_togdb;
    int		fd_fromgdb;
};

wxIMPLEMENT_APP(MyApp);

//  vim:set sw=4 ts=8: 
