#include <libimobiledevice/diagnostics_relay.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

#include <stdio.h>
#include <string.h>

void print_usage(char const *program)
{
    printf("Usage: %s <command>\n", program);
}

typedef enum {
    COMMAND_NONE,
    COMMAND_RESTART,
    COMMAND_ENTER_RECOVERY,
    COMMAND_EXIT_RECOVERY,
} command_t;

command_t parse_command(char const *command_str)
{
    if (strcmp(command_str, "restart") == 0)
        return COMMAND_RESTART;
    if (strcmp(command_str, "recovery") == 0)
        return COMMAND_ENTER_RECOVERY;
    if (strcmp(command_str, "unrecovery") == 0)
        return COMMAND_EXIT_RECOVERY;

    return COMMAND_NONE;
}

int do_enter_recovery(lockdownd_client_t lockdown)
{
    int error = lockdownd_enter_recovery(lockdown);
    if (error != LOCKDOWN_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to send device recovery.\n");
        return 1;
    }

    return 0;
}

int do_restart(idevice_t device, lockdownd_client_t lockdown)
{
    int result = 0;

    lockdownd_service_descriptor_t service = NULL;
    diagnostics_relay_client_t diagnostics = NULL;

    int error = lockdownd_start_service(lockdown, "com.apple.mobile.diagnostics_relay", &service);
    if (error != LOCKDOWN_E_SUCCESS || service == NULL || service->port <= 0) {
        fprintf(stderr, "Error: Failed to start diagnostics service.\n");
        result = 1;
        goto exit;
    }

    error = diagnostics_relay_client_new(device, service, &diagnostics);
    if (error != DIAGNOSTICS_RELAY_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to start diagnostics relay.\n");
        result = 1;
        goto exit;
    }

    error = diagnostics_relay_restart(diagnostics, DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT);
    if (error == DIAGNOSTICS_RELAY_E_SUCCESS) {
        printf("Restarting device...\n");
    } else {
        fprintf(stderr, "Error: Failed to restart device.\n");
        result = 1;
        goto exit;
    }

exit:
    if (diagnostics) {
        diagnostics_relay_goodbye(diagnostics);
        diagnostics_relay_client_free(diagnostics);
    }

    if (service)
        lockdownd_service_descriptor_free(service);

    return result;
}

int main(int argc, char const **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    command_t command = parse_command(argv[1]);

    idevice_t device = NULL;
    int error = idevice_new(&device, /*udid=*/NULL);
    if (error != IDEVICE_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to find a device.\n");
        return 1;
    }

    lockdownd_client_t lockdown = NULL;
    error = lockdownd_client_new_with_handshake(device, &lockdown, "iquick");
    if (error != LOCKDOWN_E_SUCCESS) {
        idevice_free(device);
        fprintf(stderr, "Error: Failed to open lockdown client.\n");
        return 1;
    }

    int result = 0;
    switch (command) {
    case COMMAND_RESTART:
        result = do_restart(device, lockdown);
        break;
    case COMMAND_ENTER_RECOVERY:
        result = do_enter_recovery(lockdown);
        break;
    default:
        fprintf(stderr, "Unimplemented command.\n");
        break;
    }

    if (lockdown)
        lockdownd_client_free(lockdown);
    if (device)
        idevice_free(device);

    return result;
}
