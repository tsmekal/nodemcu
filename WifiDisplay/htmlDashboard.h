const char htmlPageHeader[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char htmlPageBody[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
  <meta charset='utf-8'>
  <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css' rel='stylesheet' integrity='sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3' crossorigin='anonymous'>
  <style>
    body     { font-size:100%;} 
    .container{ margin-top: 30px;}
  </style>
 
  <title>Data</title>
  <style>
    html, body{
      background-color: #ff9900
    }
    #status{
      position: absolute;
      right: 8px;
      top: 8px;
      font-size: 0.8em;
    }
  </style>
</head>
<body>
   <div class='container mt-5'>
      <div class='row d-flex justify-content-center'>
         <div class='col-md-4 col-sm-6 col-xs-12'>
            <div class='card px-3 py-3' id='form1'>
               <span id='status'></span>
               <div class='row'>
                  <div class='col'>
                     <p>Temperature: <strong><span id='temp'></span>°C</strong></p>
                     <p>Humidity: <strong><span id='humid'></span>%</strong></p>
                     <p>Pressure: <strong><span id='pres'></span>hPa</strong></p>
                     <p>Air quality: <strong><span id='air'></span></strong></p>
                  </div>
               </div>
               <div class='row'>
                  <div class='col text-center d-grid'>
                     <button class='btn btn-outline-primary btn-sm btn-block' id='btnLED' onclick='onLedClick()'>LED</button>
                  </div>
                  <div class='col text-center d-grid'>
                     <button class='btn btn-outline-info btn-sm btn-block' id='btnDisplay' onclick='onDisplayClick()'>DISPLAY</button>
                  </div>
               </div>
               <div class='row mt-3'>
                  <div class='col text-center d-grid'>
                     <button class='btn btn-outline-danger btn-sm btn-block' id='btnResetWifi' onclick='onWifiResetClick()'>Reset WIFI configuration</button>
                  </div>
               </div>
               <div class='row mt-3 mb-0' id='lblWarning' style='display: none;'>
                  <div class='col'>
                     <div class='alert alert-danger'>Air quality is not good!</div>
                  </div>
               </div>
            </div>
         </div>
      </div>
   </div>
</body>
 
<script>
var Socket;
var interval;

function init() 
{
  document.getElementById('status').innerHTML = 'Connecting to server...';
  Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
  Socket.onmessage = function(event) { processReceivedCommand(event); };

  Socket.addEventListener('open', (event) => {  
    document.getElementById('status').innerHTML = 'Connected to server';
    interval = null;
  });
  
  Socket.addEventListener('close', (event) => {
    console.log('disconnected');
    document.getElementById('status').innerHTML = 'Connection closed';
    interval = setInterval(function(){
      init();  
    }, 5000)
  });
  
}
 
function processReceivedCommand(evt) 
{
    var data = evt.data;
    const obj = JSON.parse(data);
    var tmp = data.split('|');
    document.getElementById('temp').innerHTML = obj.temperature;  
    document.getElementById('humid').innerHTML = obj.humidity;
    document.getElementById('pres').innerHTML = obj.pressure;
    document.getElementById('air').innerHTML = obj.label + ' | VOC index: ' + obj.voc;
    // LED status
    if(obj.stateLed == 1){
      document.getElementById('btnLED').innerHTML = 'Turn OFF LED';
    }
    else{
      document.getElementById('btnLED').innerHTML = 'Turn ON LED';
    }
    // Display status
    if(obj.stateDisplay == 1){
      document.getElementById('btnDisplay').innerHTML = 'Turn OFF display';
    }
    else{
      document.getElementById('btnDisplay').innerHTML = 'Turn ON display';
    }
    if(obj.stateWarning == 1){
      document.getElementById('lblWarning').style.display = 'block';
    }
    else{
      document.getElementById('lblWarning').style.display = 'none';
    }
}
 
function sendText(data) { 
  Socket.send(data); 
}

function onLedClick(){
  sendText('LED');
}

function onDisplayClick(){
  sendText('DISPLAY');
}

function onWifiResetClick(){
	if(confirm('Really reset Wifi configuration?')){
		sendText('RESET_WIFI');
	}
}

window.onload = function(e) { 
  init(); 
}
</script>
 
</html>
)=====";
