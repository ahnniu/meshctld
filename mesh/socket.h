#ifndef SOCKET_H
#define SOCKET_H

int socket_service_init(void);
int send_back_msg(const gchar * fmt, ...) __attribute__((format(printf, 1, 2)));

// replace shell io functions
#define bt_shell_printf send_back_msg

#define bt_shell_noninteractive_quit(status) ;


#endif /* SOCKET_H */
