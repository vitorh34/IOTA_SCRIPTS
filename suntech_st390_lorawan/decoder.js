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
            
            data.DATE_TIME = hexArray.slice(9, 15).join('');
            data.LAT = hexArray.slice(15, 19).join('');
            data.LON = hexArray.slice(19, 23).join('');
            data.SPD = hexArray.slice(23, 26).join('');
            data.CRS = hexArray.slice(26, 29).join('');
            
            // Extrair SATT_FIX (sequência binária de 0's e 1's)
            data.SATT_FIX = ('00000000'+parseInt(hexArray[29], 16).toString(2)).slice(-8);
            
            data.DIST = hexArray.slice(30, 34).join('');
            data.PWR_VOLT = parseFloat(parseInt(hexArray[34], 16)+"."+hexArray[35]);
            data.IO = hexArray[36];
            data.MODE = hexArray[37];
            data.H_METER = hexArray.slice(38, 42).join('');
            data.BCK_VOLT = parseFloat(hexArray.slice(42, 44).join("."));
            data.COL_NET_RF_CH = hexArray.slice(44, 51).join('');
            
            
            // Extrair lat e lng 
            data.gps = hexToCoordinate(hexArray.slice(15, 19), hexArray.slice(19, 23), data.SATT_FIX);

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