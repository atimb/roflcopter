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
ROFL.bootload = {
    /*
     *
    */
    state: 0,
    /*
     *
    */
    progress: null,
    finish: null,
    /*
     *
    */
    load: function(hex) {
        // Intel HEX file
        
        // The binary flash data
        ROFL.bootload.flashdata = [];
        for (var j=0; j<10000; ++j) {
            ROFL.bootload.flashdata[j] = 0xFF;
        }
        
        // Convert intel HEX format to flash buffer
        var crcOK = 0;
        var crcNOK = 0;
        var lines = hex.split('\r\n');
        for (var i in lines) {
            var line = lines[i];
            ROFL.log.debug('bootload::', line);
            if (line[0] === ':') {
                // number of bytes
				var len = parseInt(line.substr(1, 2), 16);
				// read address
				var addr = parseInt(line.substr(3, 4), 16);
				// read eof shit
				var eof = parseInt(line.substr(7, 2), 16);
				if (eof != 0)
					break;
				// read actual data
				for (var j=0; j<len; ++j) {
					ROFL.bootload.flashdata[addr+j] = parseInt(line.substr(9+j*2, 2), 16);
				}
				// read CRC
				var crc = 0;
				for (var j=0; j<5+len; ++j) {
					crc += parseInt(line.substr(1+j*2, 2), 16);
				}
				crc = crc % 0x100;
				if (crc === 0) { crcOK++; }
				else { crcNOK++; }
            }
        }
        ROFL.log.info('bootload:: crcOK : ' + crcOK + ' crcNOK: ' + crcNOK);
        
        if (!(crcOK>0 && crcNOK==0)) {
		    ROFL.log.error('bootload:: Could not continue: CRC ERROR');
		    if (typeof ROFL.bootload.finish === "function") {
		        ROFL.bootload.finish({ success: false, error: 'Hex CRC error.' });
	        }
            return;
	    }
	    
	    ROFL.bootload.last_mem_addr = 10000 - 1;
	    // Find out what mem addr to upload
	    while (ROFL.bootload.flashdata[ROFL.bootload.last_mem_addr] == 0xFF) {
	        --ROFL.bootload.last_mem_addr;
	    }
	    ROFL.log.info('bootload:: Address of last byte: ' + ROFL.bootload.last_mem_addr);
	    	    
	    ROFL.log.info('bootload:: Forcing a Reset on AVR');
	    ROFL.wireless.send([0x00, 0x03, 0xFD], 1, function() {
	        ROFL.serial.onread(ROFL.bootload.loadCb);
        }, function() {
            ROFL.serial.onread(ROFL.bootload.loadCb);
        });
	    
	    ROFL.log.info('bootload:: Now waiting for PING');
	    ROFL.bootload.state = 1;
    },
    /*
     *
    */
    loadCb: function(read) {
        var read = read[0];
        switch (ROFL.bootload.state) {
            case 1:
                // Waiting PING
			    if (read === 0x05) {
			        ROFL.bootload.state = 2;
			        
                    // Replying PONG
                    ROFL.serial.write([0x06]);

				    ROFL.bootload.pageSIZE = 64;
				    ROFL.bootload.mem_addr = 0;
				    
				    ROFL.log.info('bootload:: PINGed & PONGed');
		        } else {
		            ROFL.log.warn('bootload:: Received trash instead PING');
	            }
                break;
                
            case 2:

				if (read === 'T'.charCodeAt()) { // GOOD
					ROFL.log.debug('bootload:: Received ACK!');
					if (ROFL.bootload.mem_addr > ROFL.bootload.last_mem_addr) {
					    ROFL.serial.write(':S');
				        ROFL.log.info('bootload:: Transmission over! SUCCESS!');
				        if (typeof ROFL.bootload.finish === "function") {
            		        ROFL.bootload.finish({ success: true });
            	        }
				        ROFL.bootload.state = 0;
		        	    ROFL.serial.onread(null);
				    }
				 } else {
					 if (read === 0x07) { // PACKET DROP
						 ROFL.log.warn('bootload:: Received NACK!');
						 ROFL.bootload.mem_addr -= ROFL.bootload.pageSIZE;

					 } else { // FUCK
						 ROFL.log.warn('bootload:: Received shit!');
						 ROFL.bootload.mem_addr -= ROFL.bootload.pageSIZE;
					 }
				 }

				 if (ROFL.bootload.mem_addr < 0) {
					 break;
				 }

				 var mem_addr_high = ROFL.bootload.mem_addr >> 8;
				 var mem_addr_low = ROFL.bootload.mem_addr % 256;

				 var checkSUM = 0;
				 checkSUM = ROFL.utils.hash(checkSUM, ROFL.bootload.flashdata.slice(ROFL.bootload.mem_addr, ROFL.bootload.mem_addr+ROFL.bootload.pageSIZE), ROFL.bootload.pageSIZE);
				 checkSUM = ROFL.utils.hash(checkSUM, [ROFL.bootload.pageSIZE], 1);
				 checkSUM = ROFL.utils.hash(checkSUM, [mem_addr_low], 1);
				 checkSUM = ROFL.utils.hash(checkSUM, [mem_addr_high], 1);

				 ROFL.log.debug('bootload:: Writing page: HI:'+mem_addr_high+', LO:'+mem_addr_low);
			 
                 var writeArr = [':'.charCodeAt(), ROFL.bootload.pageSIZE, mem_addr_low, mem_addr_high];
                 for (var j=0; j<4; ++j) {
					 writeArr.push((checkSUM >> (j*8)) & 0xFF);
				 }
				 for (var j=0; j<ROFL.bootload.pageSIZE; ++j) {
					 writeArr.push(ROFL.bootload.flashdata[ROFL.bootload.mem_addr + j]);
				 }       							 

				 ROFL.log.debug('bootload::', writeArr);
				 ROFL.serial.write(writeArr);

				 ROFL.bootload.mem_addr += ROFL.bootload.pageSIZE;
				 
				 if (typeof ROFL.bootload.progress === "function") {
				     ROFL.bootload.progress(100 * ROFL.bootload.mem_addr / (((ROFL.bootload.last_mem_addr >>> 6) + 1) << 6));
			     }
				 
				 break;
		}
    }
};
