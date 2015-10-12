-- Script for updating any Arkana-FTS server database (from 0.9.2 on) to the
-- newest server version (0.9.12).
--

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

SET AUTOCOMMIT=0;
START TRANSACTION;

SELECT 'Initializing update script ...' AS '';

-- Copyright (c) 2009 www.cryer.co.uk
-- Script is free to use provided this copyright header is included.
-- Modified by Lucas Beyer (Pompei2)
drop function if exists ColumnExists;
drop procedure if exists AddColumnUnlessExists;
drop procedure if exists DropColumnIfExists;
drop procedure if exists ChangeColumnIfExists;
delimiter //

create function ColumnExists(
    dbName tinytext,
    tableName tinytext,
    fieldName tinytext) returns boolean
begin
    return exists (
        SELECT * FROM information_schema.COLUMNS
        where column_name=fieldName
        and table_name=tableName
        and table_schema=dbName
        );
end;
//

create procedure AddColumnUnlessExists(
    IN dbName tinytext,
    IN tableName tinytext,
    IN fieldName tinytext,
    IN fieldDef text)
begin
    if not exists (
        SELECT * FROM information_schema.COLUMNS
        where column_name=fieldName
        and table_name=tableName
        and table_schema=dbName
        )
    then
        set @ddl=CONCAT('ALTER TABLE `',dbName,'`.`',tableName,
            '` ADD COLUMN `',fieldName,'` `',fieldDef,'`');
        prepare stmt from @ddl;
        execute stmt;
    end if;
end;
//

create procedure DropColumnIfExists(
    IN dbName tinytext,
    IN tableName tinytext,
    IN fieldName tinytext)
begin
    if exists (
        SELECT * FROM information_schema.COLUMNS
        where column_name=fieldName
        and table_name=tableName
        and table_schema=dbName
        )
    then
        set @ddl=CONCAT('ALTER TABLE `',dbName,'`.`',tableName,
            '` DROP COLUMN `',fieldName,'`');
        prepare stmt from @ddl;
        execute stmt;
    end if;
end;
//

create procedure ChangeColumnIfExists(
    IN dbName tinytext,
    IN tableName tinytext,
    IN fieldName tinytext,
    IN newFieldName tinytext,
    IN fieldDef text)
begin
    if exists (
        SELECT * FROM information_schema.COLUMNS
        where column_name=fieldName
        and table_name=tableName
        and table_schema=dbName
        )
    then
        set @ddl=CONCAT('ALTER TABLE `',dbName,'`.`',tableName,
            '` CHANGE COLUMN `',fieldName,'` `',newFieldName,'` ',fieldDef);
        prepare stmt from @ddl;
        execute stmt;
    end if;
end;
//

delimiter ;

SELECT 'Updating the users table ...' AS '';
call AddColumnUnlessExists(Database(), 'users', 'flags', 'int unsigned not NULL DEFAULT 0 AFTER `clan`');
call DropColumnIfExists(Database(), 'users', 'passwordSHA');
call DropColumnIfExists(Database(), 'users', 'rights');
call DropColumnIfExists(Database(), 'users', 'msn');
call DropColumnIfExists(Database(), 'users', 'icq');
call DropColumnIfExists(Database(), 'users', 'yahoo');
call ChangeColumnIfExists(Database(), 'users', 'address', 'contact', 'text');

SELECT 'Updating the channels table ...' AS '';
call ChangeColumnIfExists(Database(), 'channels', 'public', 'public', 'boolean not null default FALSE');

SELECT 'Updating and/or creating the user_gamepage table ...' AS '';
CREATE TABLE if not exists `user_gamepage` (
  `id` int unsigned not NULL auto_increment,
  `player` varchar(32) not NULL,
  `language` varchar(7) not NULL default 'en_US',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci;

call ChangeColumnIfExists(Database(), 'user_gamepage', 'player_id', 'player', 'varchar(32)');
call ChangeColumnIfExists(Database(), 'user_gamepage', 'language', 'language', 'varchar(7)');

-- Add the players that are not yet in that table to the table:
INSERT INTO `user_gamepage` (`player`) SELECT `users`.`nickname`
  FROM `users` LEFT JOIN `user_gamepage` ON `users`.`nickname` = `user_gamepage`.`player`
  where `user_gamepage`.`player` IS NULL;

SELECT 'Updating other tables ...' AS '';
DROP TABLE if exists `fts_news`;
DROP TABLE if exists `user_rights`;
DROP TABLE if exists `user_settings`;

-- --------------------------------------------------------

--
-- Doublure de structure pour la vue `v_channelOps`
--
SELECT 'Updating the v_channelOps view ...' AS '';

CREATE TABLE if not exists `v_channelOps` (
`nickname` varchar(32)
,`channel` varchar(32)
);
DROP TABLE if exists `v_channelOps`;

DROP VIEW if exists `v_channelOps`;
CREATE ALGORITHM=UNDEFINED VIEW `v_channelOps` AS select `users`.`nickname` AS `nickname`,`channels`.`name` AS `channel` from ((`channelOps` join `channels` on((`channelOps`.`channel` = `channels`.`id`))) join `users` on((`channelOps`.`player` = `users`.`id`)));

-- --------------------------------------------------------

--
-- Structure de la fonction `getCurrentClientVersion`
--
SELECT 'Updating the stored procedure: getCurrentClientVersion ...' AS '';

delimiter //
DROP FUNCTION if exists `getCurrentClientVersion`//
CREATE FUNCTION `getCurrentClientVersion` () returns int
DETERMINISTIC
core:begin
-- This procedure just returns the version of the client that should be used in
-- conjunction to the server right now. The version is just the major, minor and
-- sub-minor version numbers concatenated ; the build number is ignored (when we
-- officialise a version, one of major, minor, sub-minor version must change!)
--
-- For example, version 0.3.9 would become 039 or rather 39.
--
-- return: The current client version to use.
    return 123;
end//

delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `getOldestCompatibleClientVersion`
--
SELECT 'Updating the stored procedure: getOldestCompatibleClientVersion ...' AS '';

delimiter //
DROP FUNCTION if exists `getOldestCompatibleClientVersion`//
CREATE FUNCTION `getOldestCompatibleClientVersion` () returns int
DETERMINISTIC
core:begin
-- Like the name suggests, this procedure just returns the oldest version of the
-- client that is still supported with the server right now. The version is just
-- the major, minor and sub-minor version numbers concatenated ; the build
-- number is ignored (when we officialise a version, one of major, minor,
-- sub-minor version must change!)
--
-- For example, version 0.3.9 would become 039 or rather 39.
--
-- return: The oldest compatible client version.
    return 100;
end//

delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `connect`
--
SELECT 'Updating the stored function: connect ...' AS '';

delimiter //
DROP FUNCTION if exists `connect` //
CREATE FUNCTION `connect`( in_nickname varchar(32) character set utf8,
                           in_password varchar(32) character set utf8,
                           in_ip varchar(15) character set utf8,
                           in_where int ) returns int
DETERMINISTIC
begin
-- This function logs in a user: It looks if the username exists,
-- compares the MD5 password hashes, and if they match it checks
-- whether the user is already logged in. If he isn\'t, it sets the
-- ip field of the user to its ip.
--
-- This also sets the last online field of the user (on success).
--
-- param in_nickname: the nickname of the user who logs in.
-- param in_password: the MD5 encoded password of the user.
-- param in_ip: the IP address of the user.
-- param in_where: where to login the user (1 = game, 2 = site, 3 = both (=1|2))
--
-- return: 0 = logged on successfully.
--         1 = user doesn\'t exist.
--         2 = password hashes don\'t match.
--         4 = wrong password more then five times.
--         >8 = the user is already logged in.
--              (in binary, 8 = 1000, the first 1 indicates the error,
--               the two last ones correspond to in_where,
--               they mean 1=fail, 0=success).
--              So 1001 (=9) means failed to log into the game, already logged in.

    declare _pass varchar(32) character set utf8;
    declare _ip varchar(15) character set utf8;
    declare _nFails tinyint;
    declare _where int;

    SELECT `password` INTO _pass FROM `users` where `nickname`=in_nickname limit 1;
    SELECT `ip` INTO _ip FROM `users` where `nickname`=in_nickname limit 1;
    SELECT `failed_logins` INTO _nFails FROM `users` where `nickname`=in_nickname limit 1;
    SELECT `logged_in_where` INTO _where FROM `users` where `nickname`=in_nickname limit 1;

    -- Only continue if the user exists.
    if ISNULL( _pass ) then
        return 1;
    end if;

    -- Compare the passwords.
    if _pass != in_password then
        -- After the 5th failed login, reset the failed_logins field.
        SELECT _nFails+1 INTO _nFails;
        if _nFails >= 5 then
            SELECT 0 INTO _nFails;
        end if;

        UPDATE `users` SET `failed_logins`=_nFails
            where `nickname`=in_nickname limit 1;

        -- After the 5th failed login, we return 4 instead of 2.
        if _nFails = 0 then
            return 4;
        end if;

        return 2;
    end if;

    -- Check if the user is already logged in ?
    if !ISNULL( _ip ) then
        if _where & in_where != 0 then
            return 8 | (_where & in_where);
        end if;
    end if;

    -- Set the correct IP address and the in_game or in_site.
    UPDATE `users` SET `ip`=in_ip, `last_online`=NOW(), `logged_in_where`=_where|in_where, `failed_logins`=0
        where `nickname`=in_nickname and `password`=in_password limit 1;

    return 0;
end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `disconnect`
--
SELECT 'Updating the stored function: disconnect ...' AS '';

delimiter //
DROP FUNCTION if exists `disconnect` //
CREATE FUNCTION `disconnect`( in_nickname varchar(32) character set utf8,
                              in_ip varchar(15) character set utf8,
                              in_password varchar(32) character set utf8,
                              in_where int ) returns int
DETERMINISTIC
begin
-- This function logs off a user: It looks if the in_ip exists and matches
-- the in_nickname, then it compares the password hashes (in_password),
-- and if they match it checks if the user is logged in the place we want to
-- logg him out (in_where).
-- If it is the case, it loggs him out of that place and, if and only if the user
-- isn\'t logged in anywhere else, it clears its IP.
-- Else, it only sets the logged_in_where field correctly.
--
-- This also calculates the (week) connected times if logged off successfully.
--
-- param in_nickname: the nickname of the user who wants to disconnect.
-- param in_ip: the ip of the user who disconnects.
-- param in_password: the MD5 encoded password of the user.
--
-- return: 0 = logged off successfully.
--         1 = user doesn\'t exist.
--         2 = password hases don\'t match.
--         >8 = the user is not logged in.
--              (in binary, 8 = 1000, the first 1 indicates the error,
--               the two last ones correspond to in_where,
--               they mean 1=fail, 0=success).
--              So 1001 (=9) means failed to log out of the game, not logged in.
--
-- note: as far as i can understand from the mysql manual, usage
--       of the NOW and Rand functions doesn\'t make this non-deterministic.

    declare _where int;
    declare _pass varchar(32) character set utf8;
    declare _laston datetime;
    declare _onnow int unsigned;
    declare _timeweek int unsigned;
    declare _timetotal int unsigned;

    if ISNULL( in_ip ) then
        return 1;
    end if;

    SELECT `password`,`last_online`,`week_online`,`total_online`,`logged_in_where`
        INTO _pass,_laston,_timeweek,_timetotal,_where
        FROM `users`
        where `nickname`=in_nickname and `ip`=in_ip limit 1;

    -- Only continue if the user exists.
    if ISNULL( _pass ) then
        return 1;
    end if;

    -- Compare the passwords.
    if _pass != in_password then
        return 2;
    end if;

    -- Not logged in where we want to log out.
    if (_where & in_where) = 0 then
        return 8 | (_where & in_where);
    end if;

    -- Calculate the time the user was on.
    SET _onnow = UNIX_TIMESTAMP( NOW( ) ) - UNIX_TIMESTAMP( _laston );

    -- Set the IP address to NULL, update weekly online time and total online time.
    UPDATE `users` SET `last_online`=NOW( ), `week_online`=_timeweek+_onnow, `total_online`=_timetotal+_onnow, `logged_in_where`=_where^in_where, `location`=NULL
        where `nickname`=in_nickname and `ip`=in_ip limit 1;

    if (_where ^ in_where) = 0 then
        UPDATE `users` SET `ip`=NULL
            where `nickname`=in_nickname and `password`=in_password limit 1;
    end if;

    return 0;
end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `isconnected`
--
SELECT 'Updating the stored function: isconnected ...' AS '';

delimiter //
DROP FUNCTION if exists `isconnected` //
CREATE FUNCTION `isconnected`( in_ip varchar(15) character set utf8,
                               in_password varchar(32) character set utf8 ) returns int
DETERMINISTIC
begin
-- This function checks if the user is logged in and where he is. It also
-- updates various of its online activity values.
--
-- param in_ip: the ip of the user to check.
-- param in_password: the MD5 encoded password of the user to check.
--
-- return: binary code like the following:
--         00 = logged off everywhere
--         01 = logged in game only
--         10 = logged in site only
--         11 = logged in both game and site
--

    declare _where int;
    declare _nick varchar(32) character set utf8;
    declare _laston datetime;
    declare _onnow int unsigned;
    declare _timeweek int unsigned;
    declare _timetotal int unsigned;

    if ISNULL( in_ip ) then
        return 0;
    end if;

    SELECT `nickname`,`logged_in_where`,`last_online`,`week_online`,`total_online`
        INTO _nick,_where,_laston,_timeweek,_timetotal
        FROM `users`
        where `ip`=in_ip and `password`=in_password limit 1;

    -- If the user was not found, it means he is logged off everything.
    if ISNULL( _nick ) then
        return 0;
    end if;

    -- Calculate the time the user was on.
    SET _onnow = UNIX_TIMESTAMP( NOW( ) ) - UNIX_TIMESTAMP( _laston );

    -- Update the last activity.
    UPDATE `users` SET `last_online`=NOW(), `week_online`=_timeweek+_onnow, `total_online`=_timetotal+_onnow
        where `nickname`=_nick and `ip`=in_ip and `password`=in_password limit 1;

    -- So we\'re done now.
    return _where;
end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `signup`
--
SELECT 'Updating the stored function: signup ...' AS '';

delimiter //
DROP FUNCTION if exists `signup` //
CREATE FUNCTION `signup`( in_nickname varchar(32) character set utf8,
                          in_password varchar(32) character set utf8,
                          in_email varchar(64) character set utf8) returns int
DETERMINISTIC
begin
-- This function signs up user: It creates the user account and fills
-- automatically some data like the signup date etc.
--
-- param in_nickname: the nickname of the user to create.
-- param in_password: the MD5 encoded password of the user.
-- param in_email: the e-mail address of the user to create.
--
-- return: 0 = user successfully created.
--         1 = user already exists.

    declare _temp varchar(32) character set utf8;

    -- Check if the user already exists.
    SELECT `password` INTO _temp FROM `users` where `nickname`=in_nickname limit 1;

    if not ISNULL( _temp ) then
        return 1;
    end if;

    -- Create the user.
    INSERT INTO `users` (`nickname`, `password`, `e_mail`, `signup_date`)
        VALUES (in_nickname, in_password, in_email, NOW());

    INSERT INTO `user_gamepage` (`player`) VALUES (in_nickname);

    return 0;

end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `channelAddOp`
--
SELECT 'Updating the stored function: channelAddOp ...' AS '';

delimiter //
DROP FUNCTION if exists `channelAddOp` //
CREATE FUNCTION `channelAddOp`( in_user varchar(32) character set utf8,
                                in_channelID int ) returns int
DETERMINISTIC
begin
-- This function creates an entry in the table channelOps to associate
-- a player with a channel, meaning he is operator of that channel.
-- It also cares not to double entries.
--
-- param in_user: the nickname of the user to insert.
-- param in_channelID: the ID of the channel the user should be operator.
--
-- return: 0 = Entry successfully created.
--         1 = The user doesn\'t exist.
--         2 = There is already this entry.

    declare _temp int;
    declare _playerID int;

    -- Get the player\'s ID.
    SELECT `id` INTO _playerID FROM `users` where `nickname`=in_user limit 1;

    if ISNULL( _playerID ) then
        return 1;
    end if;

    -- Check if there is already an entry of this.
    SELECT `id` INTO _temp FROM `channelOps` where `player`=_playerID and `channel`=in_channelID limit 1;

    if not ISNULL( _temp ) then
        return 2;
    end if;

    -- Create the entry.
    INSERT INTO `channelOps` (`player`, `channel`) VALUES (_playerID,in_channelID);

    return 0;

end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `channelCreate`
--
SELECT 'Updating the stored function: channelCreate ...' AS '';

delimiter //
DROP FUNCTION if exists `channelCreate` //
CREATE FUNCTION `channelCreate`( in_name varchar(32) character set utf8,
                                 in_motto text character set utf8,
                                 in_admin varchar(32) character set utf8,
                                 in_public boolean ) returns int
DETERMINISTIC
begin
-- This function creates a new channel in the channels table, if such
-- a channel does not exist yet. It then returns the ID of the newly
-- created channel.
--
-- param in_name: the name of the channel to insert.
-- param in_motto: the motto of the channel to insert.
-- param in_admin: the name of the creater of the channel to insert.
-- param in_public: wether this channel has to be public (=1) or not (=0).
--
-- return: a positive integer: successfully created. The number is the ID of the channel.
--         -1 = There is already such a channel.

    declare _temp int;

    -- Check if there is already such a channel.
    SELECT `id` INTO _temp FROM `channels` where `name`=in_name limit 1;

    if not ISNULL( _temp ) then
        return -1;
    end if;

    -- Create the channel.
    INSERT INTO `channels` (`name`, `motto`, `admin`, `public`)
        VALUES (in_name, in_motto, in_admin, in_public);

    -- Get the ID of the channel.
    SELECT `id` INTO _temp FROM `channels` where `name`=in_name limit 1;

    return _temp;

end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `channelDestroy`
--
SELECT 'Updating the stored function: channelDestroy ...' AS '';

delimiter //
DROP FUNCTION if exists `channelDestroy` //
CREATE FUNCTION `channelDestroy`( in_whoAmI varchar(32) character set utf8,
                                  in_name varchar(32) character set utf8 ) returns int
DETERMINISTIC
begin
-- This function destroys a chat channel (\a in_name). That means it removes it
-- from the channels table and it removes all operators that were registered to
-- this channel.
--
-- Of course, this is only done if the guy that wants to do it (\a in_whoAmI) is
-- the administrator of that channel.
--
-- param in_whoAmI: the name of the player who wants to do this.
-- param in_name: the name of the channel to destroy.
--
-- return: 0: everything was successful
--         1: the player \a in_whoAmI has no rights to destroy that channel!
    declare _chanID int unsigned;
    declare _admin varchar(32) character set utf8;

    -- First, get the ID of that channel.
    SELECT `id`,`admin` INTO _chanID,_admin FROM `channels` where `name`=in_name limit 1;

    -- The channel doesn't exist? Then all is fine!
    if ISNULL( _chanID ) then
        return 0;
    end if;

    -- Check if that user may even destroy the channel!
    if in_whoAmI != _admin then
        return 1;
    end if;

    -- Remove all the operators.
    DELETE FROM `channelOps` where `channel`=_chanID;

    -- Remove the channel itself.
    DELETE FROM `channels` where `name`=in_name limit 1;

    return 0;
end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la procedure `init`
--
SELECT 'Updating the stored procedure: init ...' AS '';

delimiter //
DROP PROCEDURE if exists `init` //
CREATE PROCEDURE `init`( )
DETERMINISTIC
begin
-- This procedure just sets up the database for a starting masterserver.
-- That means currently it only sets every player's IP to NULL and logged_in_where
-- as not being logged in the game.

    UPDATE `users` SET `ip`=NULL;
    UPDATE `users` SET `logged_in_where`=logged_in_where & ~1;
end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la procedure `weekly_clear_online`
--
SELECT 'Updating the stored procedure: weekly_clear_online ...' AS '';

delimiter //
DROP PROCEDURE if exists `weekly_clear_online` //
CREATE PROCEDURE `weekly_clear_online`()
DETERMINISTIC
begin
-- This procedure is intended to do any stuff that has to be done once per week.
-- At the moment, it only clears every user's weekly online time.

    UPDATE `users` SET `week_online`=0;
end
//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `kick_inactive`
--
SELECT 'Updating the stored procedure: kick_inactive ...' AS '';

delimiter //
DROP PROCEDURE if exists `kick_inactive` //
CREATE PROCEDURE `kick_inactive` (in_inactivity_seconds INT)
DETERMINISTIC
begin
-- This procedure kicks any user that has not been active for more then
-- \a in_inactivity_seconds seconds. Kicks means calls the disconnect stored
-- function for the user.
-- Note that it only will disconnect the user from the gamepage, not the game!
--
-- param in_inactivity_seconds: the number of seconds that can pass before a
--                              user is considered being inactive.
--
-- return: Nothing, but selects one table per user being kicked with one column
--         being the username, the second being the return code from the
--         disconnect function.

    declare done boolean DEFAULT FALSE;
    declare _nick varchar(32) character set utf8;
    declare _pass varchar(64) character set utf8;
    declare _ip varchar(15) character set utf8;
    declare inactUsers CURSOR FOR SELECT nickname,ip,password
                                  FROM users
                                  where UNIX_TIMESTAMP(NOW()) - UNIX_TIMESTAMP(last_online) > in_inactivity_seconds
                                  and ip IS not NULL
                                  and logged_in_where & 2 = 2;
    declare CONTINUE HandLER FOR not FOUND SET done = TRUE;

    OPEN inactUsers;

    inactUsersLoop: LOOP
        FETCH inactUsers INTO _nick, _ip, _pass;

        if done then
            CLOSE inactUsers;
            LEAVE inactUsersLoop;
        end if;

        SELECT _nick,disconnect(_nick, _ip, _pass, 2);
    end LOOP;

end//
delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `getUserProperty`
--
SELECT 'Updating the stored procedure: getUserProperty ...' AS '';

delimiter //
DROP PROCEDURE if exists `getUserProperty`//
CREATE PROCEDURE `getUserProperty` (in_whoAmI varchar(32) character set utf8,
                                    in_whoseProp varchar(32) character set utf8,
                                    in_whatProp varchar(32) character set utf8)
NOT DETERMINISTIC
core:begin
-- This procedure selects one single row and column. This entry is a certain
-- property (property name \a in_whatProp, see the Dokuwiki for names) of the
-- player \a in_whoseProp.
-- It needs to know who (\a in_whoAmI) wants to get the property to check if the
-- person has the right to see the property he wants.
--
-- param in_whoAmI: The name of the player that wants to see the property.
-- param in_whoseProp: The name of the player you want to get the property from.
-- param in_whatProp: The property you want to get. See a list of properties in
--                    the dokuwiki.
--
-- return: Selects a single row and a single column (thus only one element) that
--         is the property you want. This can be NULL!
    CALL `getUserPropertyNo`(in_whoAmI, in_whoseProp,
        case in_whatProp
        when 'First name' then 5
        when 'Last name' then 6
        when 'Birthday' then 7
        when 'Sex' then 8
        when 'Comment' then 9
        when 'Jabber' then 10
        when 'Contact' then 11
        when 'IP' then 12
        when 'Location in game' then 13
        when 'Signup date' then 14
        when 'Last online date' then 15
        when 'Seconds online per week' then 16
        when 'Seconds online ever' then 17
        when 'Number of games won' then 18
        when 'Number of games lost' then 19
        when 'Number of games draw' then 20
        when 'Clan' then 21
        when 'Flags' then 22
        else 0
        end
    );
end//

delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `getUserPropertyNo`
--
SELECT 'Updating the stored procedure: getUserPropertyNo...' AS '';

delimiter //
DROP PROCEDURE if exists `getUserPropertyNo`//
CREATE PROCEDURE `getUserPropertyNo` (in_whoAmI varchar(32) character set utf8,
                                      in_whoseProp varchar(32) character set utf8,
                                      in_whatProp int)
NOT DETERMINISTIC
core:begin
-- This procedure selects one single row and column. This entry is a certain
-- property (property number \a in_prop, see the Dokuwiki for numbers) of the
-- player \a in_nickname.
--
-- It needs to know who (\a in_whoAmI) wants to get the property to check if the
-- person has the right to see the property he wants.
--
-- param in_whoAmI: The name of the player that wants to see the property.
-- param in_whoseProp: The name of the player you want to get the property from.
-- param in_whatProp: The property you want to get. See a list of properties in
--                    the dokuwiki.
--
-- Note that if you do not want to use property numbers, you can use the
--      getUserProperty stored procedure instead. You should prefer it!
--
-- return: Selects a single row and a single column (thus only one element) that
--         is the property you want. This can be NULL!
    declare _flags int;

    -- Check if the user wants to get the email from
    -- someone who doesn't want to show his email.
    -- If so, we return NULL.
    -- I can always see my own email.
    if in_whatProp=4 and in_whoAmI != in_whoseProp then
        SELECT `flags` INTO _flags FROM `users` where `nickname`=in_whoseProp limit 1;
        if _flags & 1 = 1 then
            -- Don't show the email.
            SELECT NULL;
        else
            -- Show the email.
            SELECT `e_mail` FROM `users` where `nickname`=in_whoseProp limit 1;
        end if;
        LEAVE core;
    end if;

    -- Any other property we want to show w/o special handling:
    SELECT case in_whatProp
           when 4 then `e_mail`
           when 5 then `f_name`
           when 6 then `name`
           when 7 then `bday`
           when 8 then `sex`
           when 9 then `comment`
           when 10 then `jabber`
           when 11 then `contact`
           when 12 then `ip`
           when 13 then `location`
           when 14 then `signup_date`
           when 15 then `last_online`
           when 16 then `week_online`
           when 17 then `total_online`
           when 18 then `wins`
           when 19 then `looses`
           when 20 then `draws`
           when 21 then `clan`
           when 22 then `flags`
           end
    FROM `users`
    where `nickname`=in_whoseProp limit 1;

end//

delimiter ;

-- --------------------------------------------------------

--
-- Structure de la fonction `setUserFlag`
--
SELECT 'Updating the stored procedure: setUserFlag ...' AS '';

delimiter //
DROP FUNCTION if exists `setUserFlag`//
CREATE FUNCTION `setUserFlag` (in_who varchar(32) character set utf8,
                               in_flag integer,
                               in_value boolean) returns int
DETERMINISTIC
core:begin
-- This procedure sets or clears (\a in_value) one flag (\a in_flag) of a user (\a in_who).
-- It keeps all other flags unchanged.
--
-- param in_who: The name of the player whose flag to set or clear.
-- param in_flag: Which flag to set or clear.
-- param in_value: Whether to set (true) or clear (false) the flag.
--
-- return: 0 if the flag has successfully been set or unset.
--         another value if there was an error.
    -- And set or unset that bit.
    UPDATE `users` SET `flags`=`flags` ^ ((CONVERT(-in_value, unsigned) ^ `flags`) & in_flag) where `nickname`=in_who limit 1;
    return 0;
end//

delimiter ;

SELECT 'Deinitializing update script ...' AS '';

drop function if exists ColumnExists;
drop procedure if exists AddColumnUnlessExists;
drop procedure if exists DropColumnIfExists;
drop procedure if exists ChangeColumnIfExists;

COMMIT;
