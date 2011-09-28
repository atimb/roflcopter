/**
 *  RoflCopter quadrotor project
 *  Proxy between xBee and HTML5 frontend
 *
 *  (C) 2011 Attila Incze <attila.incze@gmail.com>
 *  http://atimb.me
 *
 *  This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 * 
 */

// Require neccessary modules (express: web server, socket.io: real-time (websocket-like) browser-server communication)
var express = require('express');
var srv = express.createServer( express.static(__dirname + '/web') );
var io = require('socket.io').listen(srv);
var fs = require('fs');

var SerialPort = require("serialport").SerialPort;
var serial_port = null;

// Let our webserver listen on the specified port
srv.listen(80);

Buffer.prototype.toByteArray = function () {
    return Array.prototype.slice.call(this, 0);
}

// Handle browser connections
io.sockets.on('connection', function (socket) {
    socket.on('serial-list', function(data) {
        fs.readdir('/dev', function(err, files) {
            var serialz = [];
            for (f in files) {
                if (files[f].indexOf('usb') !== -1 && files[f].indexOf('tty') !== -1) {
                    serialz.push(files[f]);
                }   
            }
            socket.emit('serial-list', serialz);
        });
    });

    socket.on('serial-connect', function(data) {
        if (serial_port !== null) {
            serial_port.removeAllListeners('data');
            serial_port.close();
            serial_port = null;
        }
        serial_port = new SerialPort('/dev/'+data.tty, { baudrate: data.baud * 1 });
        serial_port.on('data', function (data) {
            socket.emit('serial-read-byte', data.toByteArray());
            console.log('RCVD: ', data.toByteArray());
        });
        socket.emit('serial-connected', {});
    });

    socket.on('serial-disconnect', function(data) {
        if (serial_port !== null) {
            serial_port.removeAllListeners('data');
            serial_port.close();
            serial_port = null;
        }
        socket.emit('serial-disconnected', {});
    });
    
    socket.on('serial-write-byte', function(data) {
        if (serial_port === null) {
            return;
        }
        serial_port.write(new Buffer(data));
        console.log('SENT: ', data);
    });
});
