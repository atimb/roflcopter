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
ROFL.utils = {
    /*
     *  http://en.wikipedia.org/wiki/Jenkins_hash_function
    */
    hash: function(hash, key, key_len) {
        for (var i = 0; i < key_len; i++) {
            hash += key[i];
            hash = (hash & 0xFFFFFFFF) >>> 0;
            hash += (hash << 10) >>> 0;
            hash = (hash & 0xFFFFFFFF) >>> 0;
            hash ^= (hash >>> 6);
            hash = (hash & 0xFFFFFFFF) >>> 0;
        }
        hash += (hash << 3) >>> 0;
        hash = (hash & 0xFFFFFFFF) >>> 0;
        hash ^= (hash >>> 11);
        hash = (hash & 0xFFFFFFFF) >>> 0;
        hash += (hash << 15) >>> 0;
        hash = (hash & 0xFFFFFFFF) >>> 0;
        return hash;
    }
};

/*
 *
*/
ROFL.log = {
    /*
     *  Different log levels to be used at logging initilization
    */
    DEBUG: 0,
    INFO: 1,
    WARN: 2,
    ERROR: 3,
    /*
     *
    */
    init: function(level) {
        var DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3;

        var doLog = function(prepend) {
            return function() {
                var args = [prepend].concat([].slice.call(arguments));
                return console.log.apply(console, args);
            };
        };
        var doNothing = function() {
        };

        ROFL.log.debug = level <= DEBUG ? doLog('DEBUG') : doNothing;
        ROFL.log.info = level <= INFO ? doLog('INFO') : doNothing;
        ROFL.log.warn = level <= WARN ? doLog('WARN') : doNothing;
        ROFL.log.error = level <= ERROR ? doLog('ERROR') : doNothing;
    }
}
