static int
shell_out(char * const argv[])
{
	int status;
	pid_t pid;

	pid = fork();

	if (pid < 0) {
		fprintf(stderr,
			"System error: `fork()` call failed.\n");
		exit(-1);
	}

	if (pid == 0) {
		execv(argv[0], argv);
	}

	waitpid(pid, &status, 0);
	return WEXITSTATUS(status);
}

static void
fs_copy(const char *_source, const char *dest)
{
	char *argv[5];
	char *source;
	size_t source_len;

	source = strdup(_source);
	source_len = strlen(source);

	if (source[source_len - 1] == '/')
		source[source_len - 1] = 0;

	argv[0] = "/bin/cp";
	argv[1] = "-R";
	argv[2] = source;
	argv[3] = (char *)dest;
	argv[4] = NULL;

	cl_must_pass_(
		shell_out(argv),
		"Failed to copy test fixtures to sandbox"
	);

	free(source);
}

static void
fs_rm(const char *source)
{
	char *argv[3];

	argv[0] = "/bin/rm";
	argv[1] = "-rf";
	argv[2] = (char *)source;
	argv[3] = NULL;

	cl_must_pass_(
		shell_out(argv),
		"Failed to cleanup the sandbox"
	);
}
