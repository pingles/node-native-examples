texample = require('./build/default/texample');
texample.callback = function(i) {
  console.log('Bang: ' + i);
}
texample.start();
