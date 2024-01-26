#include <libimobiledevice/diagnostics_relay.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libirecovery.h>

#undef NDEBUG
#include <assert.h>

#include <stdio.h>
#include <string.h>

void print_usage(char const *program)
{
    printf("Usage: %s [-hsrcu]\n\n", program);

    puts("Options:");
    puts("  -h        Show help and usage info");
    puts("  -e        Show the ECID of the device (in recovery)");
    puts("  -u        Show the UDID of the device");
    puts("  -s        Shut down the device");
    puts("  -r        Restart the device");
    puts("  -R        Put the device into recovery mode");
    puts("  -x        Take the device out of recovery mode");
}

typedef enum {
    COMMAND_NONE,
    COMMAND_HELP,
    COMMAND_SHOW_ECID,
    COMMAND_SHOW_UDID,
    COMMAND_SHUTDOWN,
    COMMAND_RESTART,
    COMMAND_ENTER_RECOVERY,
    COMMAND_EXIT_RECOVERY,
} command_t;

command_t parse_command(char const *command_flag)
{
#define MATCH_COMMAND(FLAG, COMMAND)         \
    do {                                     \
        if (strcmp(command_flag, FLAG) == 0) \
            return COMMAND;                  \
    } while (0)

    MATCH_COMMAND("-h", COMMAND_HELP);
    MATCH_COMMAND("-e", COMMAND_SHOW_ECID);
    MATCH_COMMAND("-u", COMMAND_SHOW_UDID);
    MATCH_COMMAND("-s", COMMAND_SHUTDOWN);
    MATCH_COMMAND("-r", COMMAND_RESTART);
    MATCH_COMMAND("-R", COMMAND_ENTER_RECOVERY);
    MATCH_COMMAND("-x", COMMAND_EXIT_RECOVERY);

    return COMMAND_NONE;
}

int do_show_udid(lockdownd_client_t lockdown)
{
    char *udid = NULL;
    int error = lockdownd_get_device_udid(lockdown, &udid);
    if (error != LOCKDOWN_E_SUCCESS)
        return 1;

    puts(udid);

    return 0;
}

int do_show_ecid(irecv_client_t client)
{
    struct irecv_device_info const *device_info = irecv_get_device_info(client);
    printf("0x%llx\n", device_info->ecid);

    return 0;
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

int do_shutdown(idevice_t device, lockdownd_client_t lockdown, int restart)
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

    if (restart)
        error = diagnostics_relay_restart(diagnostics, DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT);
    else
        error = diagnostics_relay_shutdown(diagnostics, DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT);

    if (error != DIAGNOSTICS_RELAY_E_SUCCESS) {
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

int do_lockdown_command(command_t command)
{
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
    case COMMAND_SHOW_UDID:
        result = do_show_udid(lockdown);
        break;
    case COMMAND_SHUTDOWN:
        result = do_shutdown(device, lockdown, /*restart=*/0);
        break;
    case COMMAND_RESTART:
        result = do_shutdown(device, lockdown, /*restart=*/1);
        break;
    case COMMAND_ENTER_RECOVERY:
        result = do_enter_recovery(lockdown);
        break;
    default:
        assert(0 && "Lockdown command handler received non-lockdown command!");
        break;
    }

    if (lockdown)
        lockdownd_client_free(lockdown);
    if (device)
        idevice_free(device);

    return result;
}

int do_exit_recovery(irecv_client_t client)
{
    int error = irecv_setenv(client, "auto-boot", "true");
    if (error != IRECV_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to set recovery environment variable.\n");
        return 1;
    }

    error = irecv_saveenv(client);
    if (error != IRECV_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to save environment.\n");
        return 1;
    }

    error = irecv_reboot(client);
    if (error != IRECV_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to reboot device.\n");
        return 1;
    }

    return 0;
}

int do_recovery_command(command_t command)
{

    irecv_client_t client = NULL;
    int error = irecv_open_with_ecid(&client, 0);
    if (error != IRECV_E_SUCCESS) {
        fprintf(stderr, "Error: Failed to find device in recovery.\n");
        return 1;
    }

    irecv_device_t device = NULL;
    irecv_devices_get_device_by_client(client, &device);
    if (!device) {
        fprintf(stderr, "Error: Failed to open device.\n");
        return 1;
    }

    int result = 0;
    switch (command) {
    case COMMAND_SHOW_ECID:
        result = do_show_ecid(client);
        break;
    case COMMAND_EXIT_RECOVERY:
        result = do_exit_recovery(client);
        break;
    default:
        assert(0 && "Recovery command handler got non-recovery command!");
    }

    if (client)
        irecv_close(client);

    return result;
}

int main(int argc, char const **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    command_t command = parse_command(argv[1]);
    switch (command) {
    case COMMAND_SHOW_UDID:
    case COMMAND_SHUTDOWN:
    case COMMAND_RESTART:
    case COMMAND_ENTER_RECOVERY:
        return do_lockdown_command(command);
        break;
    case COMMAND_SHOW_ECID:
    case COMMAND_EXIT_RECOVERY:
        return do_recovery_command(command);
    case COMMAND_HELP:
        print_usage(argv[0]);
        return 0;
    default:
        fprintf(stderr, "Unknown command; see `%s -h` for help.\n", argv[0]);
        return 1;
    }

    return 0;
}
