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

$(document).ready(function() {
    
    ROFL.log.init(ROFL.log.INFO);
    
    ROFL.serial.init();
    
    ROFL.serial.list(function(data) {
        for (f in data) {
            $('<option/>', { value: data[f] }).append(data[f]).appendTo('#ports');
        }
    });

    $('#connect').click(function() {
        ROFL.serial.connect($('#ports').val(), $('#baud').val(), function(data) {
            $('#connect-container').hide();
            $('#controls').show();
        });
    });
    
    $('#disconnect').click(function() {
        ROFL.serial.disconnect(function(data) {
            $('#connect-container').show();
            $('#controls').hide();
        });
    });
    
    $('#shutdown').click(function() {
        queuePackage([0x0A, 0x03, 0xF3], 1);
    });
    
    $('#gyro-compensation').change(function() {
        if ($(this).is(':checked') === true) {
            queuePackage([0x0B, 0x04, 0xF0, 0x01], 1);
        } else {
            queuePackage([0x0B, 0x04, 0xF1, 0x00], 1);
        }        
    });
    
    // Packages to be sent
    pckgQueue = [];
    var queuePackage = function() {
        pckgQueue.push($.makeArray(arguments));
        // Run queue consumer if not already
        if (queryStop) {
            queryCopter();
        }
    }
    

    var queryStop = true;
    $('#engines-display').change(function() {
        if ($(this).is(':checked') === true) {
            queryStop = false;
            queryStatus = 1;
            queryCopter();
        } else {
            queryStop = true;
        }
    });
    
    var gyroX = [], gyroY = [], gyroZ = [];
    for (var i=0; i<400; ++i) {
        gyroX[i] = 0.5;
        gyroY[i] = 0.5;
        gyroZ[i] = 0.5;
    }
    
    queryUpdateInterval = 100;

    var queryStatus = 1;
    function queryCopter() {
        if (pckgQueue.length > 0) {
            var pckg = pckgQueue.shift();
            pckg.push(queryCopter, queryCopter);
            ROFL.wireless.send.apply(this, pckg);
            return;
        }
        if (queryStop) {
            queryStatus = 1;
            return;
        }
        switch (queryStatus++) {
            case 1:
                // engine command
                ROFL.wireless.send([0x06, 0x03, 0xF7], 5, function(data) {
                    if (data[4] == 0x66) {
                        $('#engine1').val(data[0]);
                        $('#engine2').val(data[1]);
                        $('#engine3').val(data[2]);
                        $('#engine4').val(data[3]);
                    }
                    setTimeout(queryCopter, queryUpdateInterval);
                }, function() { queryStatus--; setTimeout(queryCopter, queryUpdateInterval); });
                break;
            case 2:
                // rx-control-command
                ROFL.wireless.send([0x07, 0x03, 0xF6], 9, function(data) {
                    if (data[8] == 0xBA) {
                        // Value between 0 and 1
                        var x = ((data[0]*256 + data[1]) - 1125) / 750;
                        var y = ((data[2]*256 + data[3]) - 1125) / 750;
                        var z = ((data[4]*256 + data[5]) - 1125) / 750;
                        var k = ((data[6]*256 + data[7]) - 1500) / 750;
                        /*$('#rx1').val((data[0]*256 + data[1]) - 1200);
                        $('#rx2').val((data[2]*256 + data[3]) - 1200);
                        $('#rx3').val((data[4]*256 + data[5]) - 1200);
                        $('#rx4').val((data[6]*256 + data[7]) - 1200);*/
                        // Canvas paint
                        var ctx = document.getElementById('hkrx-canvas').getContext('2d');
                        ctx.fillStyle = "rgb(238, 238, 238)";
                        ctx.fillRect(0, 0, 100, 100);
                        ctx.fillStyle = "rgb(238, 85, 85)";
                        ctx.beginPath();
                        ctx.arc(y*100, (1-z)*100, 8, 0, 2*Math.PI, true);
                        ctx.fill();

                        var ctx = document.getElementById('hkrx-canvas2').getContext('2d');
                        ctx.fillStyle = "rgb(238, 238, 238)";
                        ctx.fillRect(0, 0, 100, 100);
                        ctx.fillStyle = "rgb(238, 85, 85)";
                        ctx.beginPath();
                        ctx.arc(x*100, (1-k)*100, 8, 0, 2*Math.PI, true);
                        ctx.fill();
                        
                    }
                    setTimeout(queryCopter, queryUpdateInterval);
                }, function() { queryStatus--; setTimeout(queryCopter, queryUpdateInterval); });
                break;
            case 3:
                // gyro command
                ROFL.wireless.send([0x09, 0x03, 0xF4], 7, function(data) {
                    if (data[6] == 0xBB) {
                        // Value between 0 and 1
                        var x = ((data[0]*256 + data[1])) / 1024;
                        var y = ((data[2]*256 + data[3])) / 1024;
                        var z = ((data[4]*256 + data[5])) / 1024;
                        
                        gyroX.splice(0, 1);
                        gyroX.push(x);
                        gyroY.splice(0, 1);
                        gyroY.push(y);
                        gyroZ.splice(0, 1);
                        gyroZ.push(z);
                        
                        
                        /*$('#gyro1').val((data[0]*256 + data[1]) - 400);
                        $('#gyro2').val((data[2]*256 + data[3]) - 400);
                        $('#gyro3').val((data[4]*256 + data[5]) - 400);*/
                        
                        var ctx = document.getElementById('gyro-canvas').getContext('2d');
                        ctx.fillStyle = "#EEEEEE";
                        ctx.fillRect(0, 0, 800, 400);
                        ctx.strokeStyle = "#EE5555";
                        ctx.beginPath();
                        ctx.moveTo(0, 200);
                        for (var i=0; i<400; i++) {
                            ctx.lineTo(i*2, 200 + gyroX[i] * 1200 - 600);
                        }
                        ctx.lineTo();
                        ctx.stroke();
                        
                        ctx.strokeStyle = "#55EE55";
                        ctx.beginPath();
                        ctx.moveTo(0, 200);
                        for (var i=0; i<400; i++) {
                            ctx.lineTo(i*2, 200 + gyroY[i] * 1200 - 600);
                        }
                        ctx.lineTo();
                        ctx.stroke();
                        
                        ctx.strokeStyle = "#5555EE";
                        ctx.beginPath();
                        ctx.moveTo(0, 200);
                        for (var i=0; i<400; i++) {
                            ctx.lineTo(i*2, 200 + gyroZ[i] * 1200 - 600);
                        }
                        ctx.lineTo();
                        ctx.stroke();
                        
                    }
                    queryStatus = 1;
                    setTimeout(queryCopter, queryUpdateInterval);
                }, function() { queryStatus--; setTimeout(queryCopter, queryUpdateInterval); });
                break;
        }
    }


    $.event.props.push('dataTransfer');
    $('#bootload').bind({
        dragover: function(e) {
            $(this).css({backgroundColor: '#fdd'});
            return false;
        },
        dragleave: function(e) {
            $(this).css({backgroundColor: ''});
            return false;
        },
        drop: function(e) {
            $(this).css({backgroundColor: ''});
            var f = e.dataTransfer.files[0];
            var reader = new FileReader();
            reader.onload = function(e) {
                $('#bootload-progress').show();
                ROFL.bootload.progress = function(prog) {
                    $('#bootload-progress').val(prog);
                };
                ROFL.bootload.finish = function(resp) {
                    $('#bootload-progress').hide();
                    if (resp.success) {
                        $('#bootload-status').html('Upload success.').show().delay(3000).fadeOut();
                    } else {
                        $('#bootload-status').html('Failed: ' + resp.error).show().delay(3000).fadeOut();
                    }
                }
                ROFL.bootload.load(e.target.result);
            };
            reader.readAsBinaryString(f);
            return false;
        }
    });

});
