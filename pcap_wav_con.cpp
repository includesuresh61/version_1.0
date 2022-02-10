#include<iostream>
#include<gst/gst.h>
#include<stdio.h>
using namespace std;
#define PCMA "application/x-rtp, media=(string)audio, clock-rate=(int)8000, encoding-name=(string)PCMA, payload=(int)8"
class my_pipeline
{
	public:
		GstElement *pipeline,*pcapparse,*rtpbin,*rtppay,*dec;
		GstElement *filesrc,*convert,*resampler,*enc,*filesink;
		GstBus *bus;
		guint id;
		GstPad *dePaypad;
		GMainLoop *mainloop;
		//gst_init(NULL,NULL);
		gboolean create_element(int,int);
		gboolean link();
		static gboolean msg_handler(GstBus *,GstMessage *,gpointer);
		static GstCaps *request_pt_map_callback (GstElement * rtpbin,guint session,guint pt,gpointer udata);
                static void user_function (GstElement* rtpbin,GstPad* pad,gpointer user_data);
		void bus_watch(GMainLoop *);
		void play();
		void pause();
		void delete_resource();
		my_pipeline();
		~my_pipeline(){
		 delete_resource();
		}
};
void my_pipeline::user_function (GstElement* rtpbin,GstPad* pad,gpointer user_data){
cout<<"my_pipeline::user_function,pad--->"<<GST_PAD_NAME(pad)<<endl;
my_pipeline *pipe = static_cast<my_pipeline *>( user_data);
GstPad *peerPad=pipe->dePaypad;
if(gst_pad_link(pad,peerPad) != GST_PAD_LINK_OK){
cout<<"pad not linked"<<endl;
}


}
my_pipeline::my_pipeline():pipeline(NULL),pcapparse(NULL),rtpbin(NULL),rtppay(NULL),dec(NULL),\
		           filesrc(NULL),convert(NULL),resampler(NULL),enc(NULL),filesink(NULL){}
gboolean my_pipeline :: create_element(int src,int dst){

	pipeline=gst_pipeline_new("my-pipeline");
	filesrc=gst_element_factory_make("filesrc",NULL);
	pcapparse=gst_element_factory_make("pcapparse",NULL);
	rtpbin=gst_element_factory_make("rtpbin",NULL);
	rtppay=gst_element_factory_make("rtppcmadepay",NULL);
	dec=gst_element_factory_make("alawdec",NULL);
	convert=gst_element_factory_make("audioconvert",NULL);
	resampler=gst_element_factory_make("audioresample",NULL);
	enc=gst_element_factory_make("wavenc",NULL);
	filesink=gst_element_factory_make("filesink",NULL);
	if(  !filesrc ||!pcapparse || !rtpbin || !rtppay || !dec || !convert || !resampler || !enc || !filesink){
		cout<<"unable to create elements"<<endl;
		return FALSE;
	}
	GstCaps *cap=gst_caps_from_string(PCMA);
	g_object_set(G_OBJECT(filesrc),"location","out_out_1.pcap",NULL);
	g_object_set(G_OBJECT(pcapparse),"src-port",src,NULL);
	g_object_set(G_OBJECT(pcapparse),"dst-port",dst,NULL);
//	g_object_set(G_OBJECT(pcapparse),"caps",cap,NULL);
	g_object_set(G_OBJECT(filesink),"location","my.wav",NULL);
	g_signal_connect(G_OBJECT(rtpbin),"request-pt-map",G_CALLBACK(request_pt_map_callback),NULL);
	g_signal_connect(G_OBJECT(rtpbin),"pad-added",G_CALLBACK(user_function),this);
	return TRUE;
}
 GstCaps  *my_pipeline::request_pt_map_callback(GstElement * rtpbin,guint session,guint pt,gpointer udata){
  GstCaps *cap;
  cout<<"request_pt_map_callback,pt ---> "<<pt<<endl;
	cap=gst_caps_from_string(PCMA);
  return cap;
}
gboolean my_pipeline :: link()
{

	gst_bin_add_many(GST_BIN(pipeline),filesrc,pcapparse,rtpbin,rtppay,dec,convert,resampler,enc,filesink,NULL);
	/*if(!gst_element_link_many(filesrc,pcapparse,rtpbin,NULL)){
		cout<<"unable to link pcapparser to rtpbin"<<endl;
		return FALSE;
	}*/
	if(!gst_element_link_many(filesrc,pcapparse,rtpbin,NULL)){
		cout<<"unable to link"<<endl;
		return FALSE;
       }
	if(!gst_element_link_many(rtppay,dec,convert,resampler,enc,filesink,NULL)){
		cout<<"unable to link"<<endl;
		return FALSE;
	}
	dePaypad=gst_element_get_static_pad(rtppay,"src");
        return TRUE;
}
gboolean my_pipeline ::msg_handler(GstBus *bus,GstMessage *msg,gpointer data)
{
	my_pipeline *pipl=static_cast<my_pipeline *>(data);
         
	switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR: {
        GError *err;
        gchar *debug;

        gst_message_parse_error (msg, &err, &debug);  
	g_print ("Error: %s\n", err->message);
        g_error_free (err);
	g_free (debug);
        g_main_loop_quit (pipl->mainloop);
	delete pipl;
	break;
        }
        case GST_MESSAGE_EOS:{
        g_print("GST_MESSAGE_EOS !!!!");
        g_main_loop_quit (pipl->mainloop);
	break;
	}
        }
	return TRUE;
}
void my_pipeline::bus_watch(GMainLoop *loop)
{
        mainloop=loop; 	
	bus=gst_element_get_bus(pipeline);
        id=gst_bus_add_watch(bus,msg_handler,this);

   
}
void my_pipeline::play()
{
gst_element_set_state(pipeline,GST_STATE_PLAYING);
GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline),GST_DEBUG_GRAPH_SHOW_ALL,"my_pipe");
}
void my_pipeline::pause()
{
gst_element_set_state(pipeline,GST_STATE_NULL);
}
void my_pipeline::delete_resource()
{
cout<<"my_pipeline::delete_resource"<<endl;
g_main_loop_unref(mainloop);
gst_object_unref(bus);
if(id)
g_source_remove(id);
gst_element_set_state(pipeline,GST_STATE_NULL);
gst_object_unref(pipeline);
}
int main(int c,char *argv[])
{
	gst_init(NULL,NULL);
	GMainLoop *loop;
	/*int srcport=atoi(argv[1]);
	int dstport=atoi(argv[2]);*/
	int srcport=52212;
	int dstport=10002;
	loop = g_main_loop_new (NULL, FALSE);
	cout<<"main"<<endl;
        my_pipeline *obj=new my_pipeline();
	obj->create_element(srcport,dstport);
	obj->link();
	obj->bus_watch(loop);
	obj->play();
	g_main_loop_run (loop);


}
