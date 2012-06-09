/**
 *  RoflCopter quadrotor project
 *  HTML5 Control Interface
 *
 *  (C) 2011 Attila Incze <attila.incze@gmail.com>
 *  http://atimb.me
 *
 *  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 * 
 */

var ROFL = ROFL || {};

/*
 *
*/
ROFL.serial = {
    /*
     *
    */
    init: function(listCb, connectedCb, disconnectedCb) {
        ROFL.serial.socket = io.connect();
        ROFL.serial.socket.on('serial-read-byte', function(data) {
            if (typeof ROFL.serial.read === "function") {
                ROFL.serial.read(data);
            }
        });
    },
    /*
     *
    */
    list: function(listCb) {
        ROFL.serial.socket.on('serial-list', listCb);
        ROFL.serial.socket.emit('serial-list', {});
    },
    /*
     *
    */
    connect: function(device, baud, connectedCb) {
        ROFL.serial.socket.on('serial-connected', connectedCb);
        ROFL.serial.socket.emit('serial-connect', { tty: device, baud: baud });
    },
    /*
     *
    */
    disconnect: function(disconnectedCb) {
        ROFL.serial.socket.on('serial-disconnected', disconnectedCb);
        ROFL.serial.socket.emit('serial-disconnect', {});
    },
    /*
     *
    */
    write: function(buffer) {
        ROFL.serial.socket.emit('serial-write-byte', buffer);
    },
    /*
     *
    */
    read: null,
    /*
     *
    */
    onread: function(fn) {
        if (fn !== null && ROFL.serial.read !== null) {
            ROFL.log.error("serial:: ROFL.serial.read is already receiving for somebody. Cannot overwrite!");
            return;
        }
        ROFL.serial.read = fn;
    }
}
