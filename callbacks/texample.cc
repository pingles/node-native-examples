#include <queue>

// node headers
#include <v8.h>
#include <node.h>
#include <ev.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

using namespace node;
using namespace v8;

// handles required for callback messages
static pthread_t texample_thread;
static ev_async eio_texample_notifier;
Persistent<String> callback_symbol;
Persistent<Object> module_handle;

// message queue
std::queue<int> cb_msg_queue = std::queue<int>();
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// The background thread
static void* TheThread(void *)
{
    int i = 0;
    while(true) {
         // fire event every 5 seconds
        sleep(5);
       pthread_mutex_lock(&queue_mutex);
       cb_msg_queue.push(i);
       pthread_mutex_unlock(&queue_mutex);
       i++;
      // wake up callback
      ev_async_send(EV_DEFAULT_UC_ &eio_texample_notifier);
    }
    return NULL;
}

// callback that runs the javascript in main thread
static void Callback(EV_P_ ev_async *watcher, int revents)
{
    HandleScope scope;

    assert(watcher == &eio_texample_notifier);
    assert(revents == EV_ASYNC);

    // locate callback from the module context if defined by script
    // texample = require('texample')
    // texample.callback = function( ... ) { ..
    Local<Value> callback_v = module_handle->Get(callback_symbol);
    if (!callback_v->IsFunction()) {
         // callback not defined, ignore
         return;
    }
    Local<Function> callback = Local<Function>::Cast(callback_v);

    // dequeue callback message
    pthread_mutex_lock(&queue_mutex);
    int number = cb_msg_queue.front();
    cb_msg_queue.pop();
    pthread_mutex_unlock(&queue_mutex);

    TryCatch try_catch;

    // prepare arguments for the callback
    Local<Value> argv[1];
    argv[0] = Local<Value>::New(Integer::New(number));

    // call the callback and handle possible exception
    callback->Call(module_handle, 1, argv);

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }
}

// Start the background thread
Handle<Value> Start(const Arguments &args)
{
    HandleScope scope;

    // start background thread and event handler for callback
    ev_async_init(&eio_texample_notifier, Callback);
    //ev_set_priority(&eio_texample_notifier, EV_MAXPRI);
    ev_async_start(EV_DEFAULT_UC_ &eio_texample_notifier);
    ev_unref(EV_DEFAULT_UC);
    pthread_create(&texample_thread, NULL, TheThread, 0);

    return True();
}

void Initialize(Handle<Object> target)
{
    HandleScope scope;

    NODE_SET_METHOD(target, "start", Start);

    callback_symbol = NODE_PSYMBOL("callback");
    // store handle for callback context
    module_handle = Persistent<Object>::New(target);
}

extern "C" {
static void Init(Handle<Object> target)
{
    Initialize(target);
}

    NODE_MODULE(texample, Init);
}
