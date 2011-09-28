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
ROFL.wireless = {
    /*
     *
    */
    lastTransmissionOK : false,
    /*
     *
    */
    sending : false,
    /*
     *
    */
    send : function(buffer, respLength, cb, failcb, repeat) {
        var firstTry = (typeof repeat === 'undefined');
        if (firstTry) {
            if (ROFL.wireless.sending) {
                ROFL.log.error("wireless:: Cannot send new package, an old one is in progress.");
                return;
            }
            ROFL.wireless.sending = true;
            repeat = 3;
        } else {
            ROFL.wireless.lastTransmissionOK = false;
            ROFL.log.warn('wireless:: Trying to send again: ' + buffer);
        }
        if (repeat-- === 0) {
            ROFL.wireless.sending = false;
            ROFL.serial.onread(null);
            ROFL.log.error('wireless:: Failed to send packet:' + buffer);
            if (typeof failcb === 'function') {
                failcb();
            }
            return;
        }
        if (!ROFL.wireless.lastTransmissionOK) {
            ROFL.serial.write([0xAA, 0xAA, 0xAA, 0xAA]);
        }
        ROFL.wireless.recvBuffer = [];
        if (firstTry) {
            ROFL.serial.onread(function(data) {
                ROFL.wireless.recvBuffer = ROFL.wireless.recvBuffer.concat(data);
                if (ROFL.wireless.recvBuffer.length >= respLength) {
                    clearTimeout(ROFL.wireless.timer);
                    ROFL.serial.onread(null);
                    ROFL.wireless.sending = false;
                    ROFL.wireless.lastTransmissionOK = true;
                    ROFL.log.debug('wireless:: Packet sent & acked: ' + buffer);
                    if (typeof cb === 'function') {
                        cb(ROFL.wireless.recvBuffer);
                    }
                }
            });
        }
        ROFL.serial.write(buffer);
        ROFL.wireless.timer = setTimeout(function() { ROFL.wireless.send(buffer, respLength, cb, failcb, repeat); }, 100);
    }
}
