var hello = require('./build/default/helloworld_eio');
hi = new hello.HelloWorldEio();
console.log("Before");
hi.hello(function(data){
  console.log(data);
});
console.log("After");
