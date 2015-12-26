#ifndef D_CONFIG_H
#define D_CONFIG_H

#define DSRV_ROOT_IS_BAD "fts"

#define DSRV_MAX_LOGFILE_BYTES 1024*10240
#define DSRV_LOG_DIR "./log"
#define DSRV_FILE_NPLAYERS "nplayers.txt"
#define DSRV_FILE_NGAMES "ngames.txt"

#define DSRV_PORT_FIRST 0xAF75
#define DSRV_PORT_LAST 0xAF7F

#define DSRV_MYSQL_HOST "localhost"
#define DSRV_MYSQL_USER "fts"
#define DSRV_MYSQL_PASS "fts-server"
#define DSRV_MYSQL_DB "arkana_game"

#define DSRV_DEFAULT_CHANNEL_NAME "Talk To Survive (main channel)"
#define DSRV_DEFAULT_CHANNEL_MOTTO "This is the main channel in Arkana-FTS where everybody meets"
#define DSRV_DEFAULT_CHANNEL_ADMIN "Pompei2"
#define DSRV_DEVS_CHANNEL_NAME "Dev's Channel"
#define DSRV_DEVS_CHANNEL_MOTTO "The channel for the developers of Arkana-FTS and the players wanting to know more."
#define DSRV_DEVS_CHANNEL_ADMIN "Pompei2"
#define DSRV_DEFAULT_MOTTO "Arkana-FTS Rules!"
#define DSRV_MAX_CHANS_PER_USER 3

#endif // D_CONFIG_H

