#include <libimobiledevice/diagnostics_relay.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

#include <stdio.h>

int main(int argc, char const **argv)
{
    (void)argc;
    (void)argv;

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

    lockdownd_service_descriptor_t service = NULL;
    error = lockdownd_start_service(lockdown, "com.apple.mobile.diagnostics_relay", &service);
    if (error != LOCKDOWN_E_SUCCESS || service == NULL || service->port <= 0) {
        idevice_free(device);
        fprintf(stderr, "Error: Failed to start diagnostics service.\n");
        return 1;
    }

    lockdownd_client_free(lockdown);

    diagnostics_relay_client_t diagnostics = NULL;
    error = diagnostics_relay_client_new(device, service, &diagnostics);
    if (error != DIAGNOSTICS_RELAY_E_SUCCESS) {
        idevice_free(device);
        fprintf(stderr, "Error: Failed to start diagnostics relay.\n");
        return 1;
    }

    error = diagnostics_relay_restart(diagnostics, DIAGNOSTICS_RELAY_ACTION_FLAG_WAIT_FOR_DISCONNECT);
    if (error == DIAGNOSTICS_RELAY_E_SUCCESS) {
        printf("Restarting device...\n");
    } else {
        idevice_free(device);
        fprintf(stderr, "Error: Failed to restart device.\n");
        return 1;
    }

    idevice_free(device);

    return 0;
}
