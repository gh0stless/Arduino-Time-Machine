/**************************************************************************************************
** Network functions                                                                             **
***************************************************************************************************/

//the NTP server stuff
void processNTP() {

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    Udp.read(packetBuffer,NTP_PACKET_SIZE);
    IPAddress Remote = Udp.remoteIP();
    int PortNum = Udp.remotePort();

    packetBuffer[0] = 0b00100100;   // LI, Version, Mode
    packetBuffer[1] = 1 ;   // stratum
    packetBuffer[2] = 6 ;   // polling minimum
    packetBuffer[3] = 0xFA; // precision

    packetBuffer[7] = 0; // root delay
    packetBuffer[8] = 0;
    packetBuffer[9] = 8;
    packetBuffer[10] = 0;

    packetBuffer[11] = 0; // root dispersion
    packetBuffer[12] = 0;
    packetBuffer[13] = 0xC;
    packetBuffer[14] = 0;
    
    tempval = timestamp;

    packetBuffer[12] = 71; //"G";
    packetBuffer[13] = 80; //"P";
    packetBuffer[14] = 83; //"S";
    packetBuffer[15] = 0; //"0";

    // reference timestamp
    packetBuffer[16] = (tempval >> 24) & 0XFF;
    tempval = timestamp;
    packetBuffer[17] = (tempval >> 16) & 0xFF;
    tempval = timestamp;
    packetBuffer[18] = (tempval >> 8) & 0xFF;
    tempval = timestamp;
    packetBuffer[19] = (tempval) & 0xFF;

    packetBuffer[20] = 0;
    packetBuffer[21] = 0;
    packetBuffer[22] = 0;
    packetBuffer[23] = 0;

    //copy originate timestamp from incoming UDP transmit timestamp
    packetBuffer[24] = packetBuffer[40];
    packetBuffer[25] = packetBuffer[41];
    packetBuffer[26] = packetBuffer[42];
    packetBuffer[27] = packetBuffer[43];
    packetBuffer[28] = packetBuffer[44];
    packetBuffer[29] = packetBuffer[45];
    packetBuffer[30] = packetBuffer[46];
    packetBuffer[31] = packetBuffer[47];

    //receive timestamp
    packetBuffer[32] = (tempval >> 24) & 0XFF;
    tempval = timestamp;
    packetBuffer[33] = (tempval >> 16) & 0xFF;
    tempval = timestamp;
    packetBuffer[34] = (tempval >> 8) & 0xFF;
    tempval = timestamp;
    packetBuffer[35] = (tempval) & 0xFF;

    packetBuffer[36] = 0;
    packetBuffer[37] = 0;
    packetBuffer[38] = 0;
    packetBuffer[39] = 0;

    //transmitt timestamp
    packetBuffer[40] = (tempval >> 24) & 0XFF;
    tempval = timestamp;
    packetBuffer[41] = (tempval >> 16) & 0xFF;
    tempval = timestamp;
    packetBuffer[42] = (tempval >> 8) & 0xFF;
    tempval = timestamp;
    packetBuffer[43] = (tempval) & 0xFF;

    packetBuffer[44] = 0;
    packetBuffer[45] = 0;
    packetBuffer[46] = 0;
    packetBuffer[47] = 0;

    // Reply to the IP address and port that sent the NTP request
    Udp.beginPacket(Remote, PortNum);
    Udp.write(packetBuffer,NTP_PACKET_SIZE);
    Udp.endPacket();
  }
}

void processWWW() { 
    // listen for incoming clients
    EthernetClient client = server.available();

    if (client) {
        #if debug      
          Serial.println("New client");
        #endif
        boolean currentLineIsBlank = true; // an http request ends with a blank line

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
#if debug
                //Serial.write(c);
#endif
                // if you've gotten to the end of the line (received a newline character) and
                // the line is blank, the http request has ended, so you can send a reply.
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");  // the connection will be closed after completion of the response
                    client.println("Refresh: 60");  // refresh the page automatically every min
                    client.println();
                    client.println("<!DOCTYPE HTML>");
                    client.println("<html><head><title>Arduino Webserver</title><meta charset=\"UTF-8\" /></head>");
                    client.println("<body><h1>Arduino Webserver</h1><ul>");
               
                    client.print(hour());
                    client.print(":");
                    if(minute() < 10) client.print("0");
                    client.print(minute());
                    client.print(":");
                    if(second() < 10) client.print("0");
                    client.print(second());
                    client.print(" ");
                    client.print(day());
                    client.print(".");
                    client.print(month());
                    client.print(".");
                    client.print(year());
                    client.print(" ");
                    switch (weekday())
                    {
                      case 2:
                        client.print("MON");
                        break;
                      case 3:
                        client.print("TUE");
                        break;
                      case 4:
                        client.print("WED");
                        break;
                      case 5:
                        client.print("THU");
                        break;
                      case 6:
                        client.print("FRI");
                        break;
                      case 7:
                        client.print("SAT");
                        break;
                      case 1:
                        client.print("SUN");
                        break;
                    }
                   
                    //for (byte pin = 0; pin < 6; pin++) {
                    //    client.println("<li>A" + String(pin) + " = " + String(analogRead(pin)) + "</li>");
                    //}                 
                   
                    client.println("</ul></body></html>");
                    break;
                }

                if (c == '\n') {
                    currentLineIsBlank = true; // you're starting a new line
                } else if (c != '\r') {
                    currentLineIsBlank = false; // you've gotten a character on the current line
                }
            }
        }

        // give the web browser time to receive the data
        delay(100); // 1?

        client.stop();
#if debug
        //Serial.println("disconnected.");
#endif
    }
}
