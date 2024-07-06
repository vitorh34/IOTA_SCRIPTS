function hexToCoordinate(latHexArray, lngHexArray, SATT_FIX){
    stringLatCoordinate = parseInt(latHexArray[0], 16)+"."+latHexArray.slice(1, 4).join("");
    stringLngCoordinate = parseInt(lngHexArray[0], 16)+"."+lngHexArray.slice(1, 4).join("");
    
    unsignedLat = parseFloat(stringLatCoordinate);
    unsignedLng = parseFloat(stringLngCoordinate);
    
    var gps = {
        "lat": SATT_FIX[1] == 0 ? unsignedLat: unsignedLat * -1,
        "lng": SATT_FIX[2] == 0 ? unsignedLng: unsignedLng * -1,
    };
    return gps;
}


//function to parse hexString to hexArray
function hexStringToHexArray(hexString) {
    // Remova qualquer prefixo "0x" se estiver presente
    hexString = hexString.replace(/^0x/i, '');
    // Divida a string hexadecimal em pares de dois caracteres
    const hexPairs = hexString.match(/.{2}/g);
    if (!hexPairs) {
        return []; // Retorne um array vazio se não houver pares válidos
    }
    // Retorne o array de pares hexadecimais
    return hexPairs;
}
    
    

function decodeUplink(input) {
    var port = input.fPort;
    var bytes = input.bytes;
    var raw = input.raw;
    var hexArray = hexStringToHexArray(raw);
    var data = {};
    switch (input.fPort) {
        case 1:
            // Extrair HDR (byte 0)
            data.HDR = hexArray[0];

            // Extrair DEV_ID
            data.DEV_ID = hexArray.slice(1, 6).join("");
        
            // Extrair MODEL
            data.MODEL = hexArray[6];
            
            // Extrair Software Version
            data.SW_VER = hexArray.slice(7, 9).join('.');
            
            data.DATE_TIME = hexArray.slice(24, 30).join('');
            data.LAT = hexArray.slice(30, 34).join('');
            data.LON = hexArray.slice(34, 38).join('');

            data.SPD = hexArray.slice(38, 41).join('');
            data.CRS = hexArray.slice(41, 44).join('');
            
            // Extrair SATT_FIX (sequência binária de 0's e 1's)
            data.SATT_FIX = ('00000000'+parseInt(hexArray[44], 16).toString(2)).slice(-8);
            
            // Extrair lat e lng 
            data.gps = hexToCoordinate(hexArray.slice(30, 34), hexArray.slice(34, 38), data.SATT_FIX);

            return {
                data: data
                }
            break;
        default:
            return {
                errors: ["unknown FPort"]
            }
    }
}
                                                        
                                                        