function getLocation() {
  navigator.geolocation.getCurrentPosition(
    function(position) {
      console.log("Hello: " + position.coords.latitude + "," + position.coords.longitude);
      Pebble.sendAppMessage({
        "latitude": "" + position.coords.latitude,
        "longitude": "" + position.coords.longitude});
    },
    function(error) {
      console.log(error.message);
      Pebble.sendAppMessage({
        "latitude": "Error: " + error.message,
        "longitude": "Error: " + error.message});
    },
    {maximumAge: 600000});
}

// Set callback for the app ready event
Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect! ");
                          getLocation();
                        });

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
                        function(e) {
                          console.log("Message: " + e.payload.location);
                          getLocation();
                        });

