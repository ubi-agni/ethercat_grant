/**
 * This code is taken from pr2-grant
 * https://code.ros.org/svn/pr2debs/trunk/packages/pr2/pr2-grant/pr2_grant.cpp
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/capability.h>
#include <string>
#include <cstdlib>
#include <sys/prctl.h>

using namespace std;

#define EXECUTABLE "/var/tmp/granted"

int main(int argc, char *argv[])
{
  // parse options
  unsigned int args_offset = 1;
  if (argc > 1) {
    if (argv[1][0] == '-') {
      if (argv[1][1] == 'e') 
      {
        if (argc > 3)
        {
          // add the passed argument to the environment
          if (putenv(argv[2])!=0)
          {
            perror("invalid env");
            return -1;
          }
          args_offset = 3;
        }
        else
        {
          printf("  error: option -e requires an argument in form <VAR>=<string>\n");
          return -1;
        }
      }
      if (argv[1][1] == 'h') 
      {
        printf("syntax: ethercat_grant [-e <VAR>=<string> | -h] <application> [args]\n\n -e is an option to pass a single environment variable added to environment of the executable\n");
        return 0;
      }
    }
  }

  // Remove old executable, if it exists
  unlink(EXECUTABLE);

  // Copy new executable to /var/tmp
  string cmd;
  cmd = string("/bin/cp ") + string(argv[args_offset]) + string(" " EXECUTABLE);
  if (system(cmd.c_str()) == -1) {
    perror("cp");
    return -1;
  }
  if (chown(EXECUTABLE, getuid(), getgid()) < 0) {
    perror("chown");
    return -1;
  }

  // Create capability set
  const char *cap_text = "cap_ipc_lock=ep cap_net_raw=ep cap_sys_nice=ep cap_net_admin=ep";
  cap_t cap_d = cap_from_text(cap_text);
  if (cap_d == NULL) {
    perror("cap_from_text");
    return -1;
  }

  // Set file capability
  int retval = cap_set_file(EXECUTABLE, cap_d);
  if (retval != 0) {
    fprintf(stderr, "Failed to set capabilities on file `%s' (%s)\n", argv[args_offset], strerror(errno));
    return -1;
  }

  // Free capability
  if (cap_d) {
    cap_free(cap_d);
  }

  // Drop privileges
  retval = setuid(getuid());
  retval = setgid(getgid());

  // Allow core dumps
  prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

  // Exec executable
  if (execve(EXECUTABLE, argv + args_offset, environ) < 0) {
    perror("execve");
    return -1;
  }

  return 0;
}
