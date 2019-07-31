#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>

#include "mesh/cmds.h"

GMainLoop *loop = NULL;
GSocketConnection * g_cn = NULL;


int send_back_msg(const gchar * fmt, ...)
{
	va_list args;
	gchar buffer[1024];
	gsize len = 0;

	if(g_cn == NULL){
		va_start(args, fmt);
		vprintf(fmt,args);
		va_end(args);
		return FALSE;
	}

        va_start(args, fmt);
        vsprintf(buffer,fmt,args);
        va_end(args);

	len = strlen(buffer);

	GOutputStream * out_stream = g_io_stream_get_output_stream(G_IO_STREAM((GSocketConnection *)g_cn));
	gssize ret_int = g_output_stream_write(out_stream, buffer, len, NULL, NULL);
	g_output_stream_flush(out_stream, NULL, NULL);

	if (ret_int < 1) {
		g_error("write error");
	}

	return TRUE;
}

gboolean read_msg(GIOChannel *channel, GIOCondition condition, gpointer data)
{
    gsize len = 0;
    gchar *buffer = NULL;
    GError *error=NULL;

    GIOStatus ret = g_io_channel_read_line(channel, &buffer, &len, NULL, NULL);
    if (ret == G_IO_STATUS_ERROR){
        g_error ("Error reading: %s\n", error->message);
        g_object_unref(data);
        return FALSE;
    }
    else if (ret == G_IO_STATUS_EOF) {
        g_print("client finished\n");
        return FALSE;
    }
    else {
        if(len > 0) {
            if ('\n' == buffer[len -1]) {
                buffer[len -1] = '\0';
            }
            g_print("rev: %s\n", buffer);
		parse_cmds(buffer);
        }
        if(NULL != buffer) {
            //判断结束符
            if(strcasecmp(buffer, "q") == 0){
                g_main_loop_quit(loop);
            }
        }

        g_free(buffer);
    
        return TRUE;
    }
}

gboolean
new_connection(GSocketService *service,
              GSocketConnection *connection,
              GObject *source_object,
              gpointer user_data)
{
    // tell glib not to disconnect
    g_object_ref(connection);
    GSocketAddress *sockaddr = g_socket_connection_get_remote_address(connection, NULL);
    GInetAddress *addr = g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(sockaddr));
    guint16 port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(sockaddr));

    g_print("New Connection from %s:%d\n", g_inet_address_to_string(addr), port);

    //read client
    GSocket *socket = g_socket_connection_get_socket(connection);
    gint fd = g_socket_get_fd(socket);
    GIOChannel *channel = g_io_channel_unix_new(fd);
    if(channel) {
        g_io_add_watch(channel, G_IO_IN, (GIOFunc) read_msg, connection);
        g_io_channel_unref(channel);
    }
	// set global connection
	g_cn = connection;

    return TRUE;
}



int socket_service_init(void) 
{
    GSocketService *service = g_socket_service_new();
    GInetAddress *address = g_inet_address_new_from_string("127.0.0.1");
    GSocketAddress *socket_address = g_inet_socket_address_new(address, 4000);
    g_socket_listener_add_address(G_SOCKET_LISTENER(service), socket_address, G_SOCKET_TYPE_STREAM,
            G_SOCKET_PROTOCOL_TCP, NULL, NULL, NULL);

    g_object_unref(socket_address);
    g_object_unref(address);
    g_socket_service_start(service);

    g_signal_connect(service, "incoming", G_CALLBACK(new_connection), NULL);
}

