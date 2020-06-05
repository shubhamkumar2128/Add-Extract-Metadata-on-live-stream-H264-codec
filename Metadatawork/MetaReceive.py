
import gi 
import sys
gi.require_version('Gst', '1.0')
gi.require_version('Gtk', '3.0')
from gi.repository import Gst, GObject
import cv2
import numpy

class Receive:

	count=0

	def probe_cb(self,pad, info,pasd):  #metadata callback function
		print("######################Metadata##########################")
		buf=info.get_buffer()
		ret,map_info = buf.map(Gst.MapFlags.READ)
		buf_size = buf.get_size()
		print("Size of orignal buffer : ", buf_size)	

		cameraId = map_info.data[buf_size-17:buf_size-25:-1]
		cameraId=[ ord(i) for i in cameraId]
		cameraId=['{0:08b}'.format(i) for i in cameraId]
		cameraId=''.join(cameraId)
		cameraId=int(cameraId,2)
		print("Camera_id : ",cameraId)

		sec = map_info.data[buf_size-9:buf_size-17:-1]
		sec=[ ord(i) for i in sec]
		sec=['{0:08b}'.format(i) for i in sec]
		sec=''.join(sec)
		sec=int(sec,2)
		print("Second : ",sec)

		usec = map_info.data[buf_size:buf_size-9:-1]
		usec=[ ord(i) for i in usec]
		usec=['{0:08b}'.format(i) for i in usec]
		usec=''.join(usec)
		usec=int(usec,2)
		print(" Microsecond : ",usec)
		return Gst.PadProbeReturn.OK

	def on_message(self,bus, message, loop):
    		mtype = message.type 
    		if mtype == Gst.MessageType.EOS:
      		  	print("End of stream") 
      		 	loop.quit()
   	        elif mtype == Gst.MessageType.ERROR: 
        		err, debug = message.parse_error() 
        		print(err, debug) 
        		loop.quit()
    		elif mtype == Gst.MessageType.WARNING: 
        		err, debug = message.parse_warning() 
        		print(err, debug) 
	        return True
	
	def stop(self):
		self.pipeline.set_state(Gst.State.NULL)
        
	def __init__(self):
		Gst.init(None)
		self.pipeline = Gst.Pipeline() 
		self.frame = None

		netrcv = Gst.ElementFactory.make("udpsrc","netrcv")         	
		netrcv.set_property("port", 5009)
        	caps = Gst.Caps.from_string("application/x-rtp, media=(string)application, clock-rate=(int)90000, encoding-name=(string)X-GST, capsversion=(string)0, payload=(int)96")
        	capfltr = Gst.ElementFactory.make("capsfilter", "filter1") 
        	capfltr.set_property("caps", caps)

        	rtph = Gst.ElementFactory.make("rtpgstdepay") 

        	avdec = Gst.ElementFactory.make("avdec_h264") 

        	videocon = Gst.ElementFactory.make("videoconvert") 

		sink=Gst.ElementFactory.make("autovideosink") 
		
        #add elements
		self.pipeline.add(netrcv)
		self.pipeline.add(capfltr)
		self.pipeline.add(rtph)
		self.pipeline.add(avdec)
		self.pipeline.add(videocon)
		self.pipeline.add(sink)
		
        #link elements
		if not Gst.Element.link(netrcv, capfltr):
   		 	print("Elements could not be linked.")
    			exit(-1)
		if not Gst.Element.link(capfltr, rtph):
    			print("Elements could not be linked.")
    			exit(-1)
		if not Gst.Element.link(rtph, avdec):
    			print("Elements could not be linked app.")
    			exit(-1)
		if not Gst.Element.link(avdec, videocon):
    			print("Elements could not be linked app.")
    			exit(-1)
		if not Gst.Element.link(videocon, sink):
    			print("Elements could not be linked app.")
    			exit(-1)
		
		#callback for metadata
		pad=avdec.get_static_pad("sink")
		pad.add_probe(Gst.PadProbeType.BUFFER, self.probe_cb,None)

		self.pipeline.set_state(Gst.State.PLAYING)	

	        loop = GObject.MainLoop() 
            	try: 
               	 loop.run()
            	except: 
               	 loop.quit()
           	bus = self.pipeline.get_bus()
            	bus.connect("message", self.on_message, None)
	

video = Receive()

