#include "net.h"

#ifdef BYTE_WIN32
    #include <winsock2.h>
    #pragma comment( lib, "wsock32.lib" )

    #define close closesocket

    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <net/if.h>
    #include <fcntl.h>
    #include <ifaddrs.h>
#endif
 
/// Application Protocol ID. Just the hash of "Byte-Project"
const u32 protocol_id = 1910167069;

const f32 connection_timeout = 10.f;

net_ip local_ips[MAX_IPS];
static int numIP;



  

static void add_local_ip( char *ifname, struct sockaddr *addr, struct sockaddr *mask ) {
    int addrlen;
    sa_family_t family;

    if( !ifname || !addr || !mask )
        return;

    family = addr->sa_family;

    if( numIP < MAX_IPS && family == AF_INET ) {
        addrlen = sizeof(struct sockaddr_in);

        strncpy( local_ips[numIP].ifname, ifname, 16 );
        local_ips[numIP].family = family;
        memcpy( &local_ips[numIP].addr, addr, addrlen );
        memcpy( &local_ips[numIP].mask, mask, addrlen );

        numIP++;
    }
}

static void show_ip() {
    char buf[48];

    for( int i = 0; i < numIP; ++i ) {
        if( getnameinfo((struct sockaddr*)&local_ips[i].addr, sizeof(struct sockaddr_in), buf, 48, NULL, 0, NI_NUMERICHOST ) )
            buf[0] = 0;

        log_info( "IP : %s (%s)\n", buf, local_ips[i].ifname );
    }
}


static void get_local_ips() {
    struct ifaddrs *ifap, *search;

    if( getifaddrs( &ifap ) ) 
        printf( "Unable to get list of net interfaces\n" );
    else {
        numIP = 0;
        for( search = ifap; search; search = search->ifa_next ) {
            if( search->ifa_flags & 0x01 ) 
                add_local_ip( search->ifa_name, search->ifa_addr, search->ifa_netmask );
        }
        freeifaddrs( ifap );
    }

    show_ip();
}


bool net_addr_equal( const net_addr *a, const net_addr *b ) {
    if( !a || !b ) return false;

    u32 ipa, ipb;
    bytes_to_u32( a->ip, &ipa );
    bytes_to_u32( b->ip, &ipb );

    return ipa == ipb && a->port == b->port;
}

void net_addr_fill( net_addr *a, u8 ip1, u8 ip2, u8 ip3, u8 ip4, u16 port ) {
    if( a ) {
        a->ip[0] = ip1;
        a->ip[1] = ip2;
        a->ip[2] = ip3;
        a->ip[3] = ip4;
        a->port = port;
    } else
        log_err( "Given address receiver not allocated\n" );
}

void net_addr_fill_int( net_addr *a, u32 ip, u16 port ) {
    if( a ) {
        u32_to_bytes( ip, a->ip );
        a->port = port;
    } else
        log_err( "Given address receiver not allocated\n" );
}

bool net_init() {
    bool ret = false;

#ifdef BYTE_WIN32
    WSAData wsa_data;
    ret = WSAStartup( MAKEWORD( 2,2 ), &wsa_data ) != NO_ERROR;
#else
    ret = true;
#endif
 
    get_local_ips();

    return ret;
}

void net_shutdown() {
#ifdef BYTE_WIN32
    WSACleanup();
#endif
}



////////////////////////////////////////////////////////////////////////////
//                      PACKET QUEUE

/// Returns true if sequence a is more recent than sequence b
/// Takes into account the fact that buffers cycle after MAX_SEQUENCE
static bool sequence_more_recent( u32 a, u32 b ) {
    return ( a > b && ( a - b ) <= MAX_SEQUENCE/2 )
        || ( a < b && ( b - a ) >  MAX_SEQUENCE/2 );
}

void net_packet_queue_init( net_packet_queue *q ) {
    if( q ) {
        q->count = 0;
        q->tail = 0;
        for( int i = 0; i < 256; ++i )
            q->list[i] = &q->arr[i];
    }
}

void net_packet_queue_insert( net_packet_queue *q, net_packet_info *p ) {
    if( !q || !p ) return;

    if( !q->count ) {
        memcpy( &q->arr[0], p, sizeof(net_packet_info) );
        ++q->count;

    } else {
        bool full = false;

        if( 256 == q->count )
            full = true;

        if( sequence_more_recent( q->list[q->tail]->seq, p->seq ) ) {
            if( !full ) {
                memcpy( q->list[q->count], p, sizeof(net_packet_info) );
                q->tail = q->count++;
            }
            // dont handle the case where arr is full
            return;
        }

        u16 i;
        for( i = 0; i < q->count; ++i ) {
            if( sequence_more_recent( p->seq, q->list[i]->seq ) ) {
                // we have space, put new one at first available spot and redo linking
                if( !full ) {
                    memcpy( q->list[q->count], p, sizeof(net_packet_info) );
                    net_packet_info *tmp = &(*q->list[q->count]);
                    q->list[q->count] = q->list[q->tail];

                    for( int j = q->count-1; j > i; --j ) 
                        q->list[j] = q->list[j-1];

                    q->list[i] = tmp;
                    q->tail = q->count++;

                // no more space in array, replace the older packet with the newone
                } else {
                    memcpy( q->list[q->tail], p, sizeof(net_packet_info) );
                    net_packet_info *tmp = q->list[q->tail];

                    for( int j = q->tail; j > i; --j ) 
                        q->list[j] = q->list[j-1];

                    q->list[i] = tmp;
                }
                return;
            }   
        }
        // error
        printf( "ARR ERROR\n" );
    }
}

void net_packet_queue_remove( net_packet_queue *q, u32 index ) {
    if( !q ) return;

    for( u32 i = index; i < q->tail; ++i ) {
        q->list[i] = q->list[i+1];
    }

    q->list[q->tail] = &q->arr[index];
    q->list[q->tail]->seq = 0;
    q->count--;
    q->tail--;
}

bool net_packet_queue_exists( net_packet_queue *q, u32 seq ) {
    if( !q ) return false;

    for( int i = 0; i < q->count; ++i ) 
        if( q->arr[i].seq == seq )
            return true;

    return false;
}

void net_packet_queue_verify( net_packet_queue *q );

void net_packet_queue_print( net_packet_queue *q ) {
    if( !q ) return;

    for( int i = 0; i < q->count; ++i )
        printf( " + %d\n", q->list[i]->seq );
}

bool net_packet_queue_empty( net_packet_queue *q ) {
    return q && q->count;
}

void net_packet_queue_update( net_packet_queue *q, f32 dt ) {
    if( !q ) return;

    for( u32 i = 0; i < q->count; ++i )
        q->arr[i].time += dt;
}

////////////////////////////////////////////////////////////////////////////
//                      CONNECTION

static void net_connection_clear( connection_t *c ) {
    c->state = Disconnected;
    c->timeout_accum = 0.f;
    memset( c->address.ip, 0, 4 );
    c->address.port = 0;
}

bool net_connection_init( connection_t *c, connection_mode mode, u16 port ) {
    check( c, "Given connection not allocated\n" );

    c->running = false;
    c->socket = 0;
    c->mode = mode;

    c->seq_local = 0;
    c->seq_remote = 0;

    net_packet_queue_init( &c->sent_queue );
    net_packet_queue_init( &c->recv_queue );
    net_packet_queue_init( &c->pending_acks );
    net_packet_queue_init( &c->ackd_queue );

    c->sent_packets = 0;
    c->recv_packets = 0;
    c->lost_packets = 0;
    c->ackd_packets = 0;

    c->sent_bw = 0.f;
    c->ackd_bw = 0.f;
    c->rtt= 0.f;

    net_connection_clear( c );

    bool noerr = net_open_socket( &c->socket, port );
    check( noerr, "Failed to open connection socket\n" );

    c->running = true;

    return true;

error:
    net_connection_shutdown( c );
    return false;
}

void net_connection_shutdown( connection_t *c ) {
    if( c && c->running ) {
        net_connection_clear( c );
        c->running = false;

        if( c->socket )
            net_close_socket( &c->socket );
    }
}

void net_connection_listen( connection_t *c ) {
    if( !c ) return;


    if( c->mode == Server )
        log_info( "Server Listening...\n" );
    
    if( c->mode == Client )
        log_info( "Client getting in Listening mode. WTF?\n" );

    c->state = Listening;
}

void net_connection_connect( connection_t *c, net_addr *addr ) {
    if( !c || !addr ) return;

    log_info( "Connecting to %d.%d.%d.%d:%d\n", 
            addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3], addr->port ); 

    c->state = Connecting;
    memcpy( &c->address, addr, sizeof(net_addr) );
}

void net_connection_update( connection_t *c, f32 dt ) {
    if( !c || !c->running ) return;

    // check connection timeout
    c->timeout_accum += dt;
    if( c->timeout_accum > connection_timeout ) {
        if( c->state == Connecting ) {
            log_info( "Connecting timed out\n" );
            net_connection_clear( c );
            c->state = ConnectFail;
        } else if( c->state == Connected ) {
            log_info( "Connection timed out\n" );
            net_connection_clear( c );
        }
        return;
    }

    // update queues packet time
    net_packet_queue_update( &c->sent_queue, dt );
    net_packet_queue_update( &c->recv_queue, dt );
    net_packet_queue_update( &c->ackd_queue, dt );
    net_packet_queue_update( &c->pending_acks, dt );

    // remove outdated packets from all queues
    while( !net_packet_queue_empty( &c->sent_queue ) && c->sent_queue.arr[c->sent_queue.tail].time > (1.f + M_EPS) )
        c->sent_queue.count--;

    if( !net_packet_queue_empty( &c->recv_queue ) ) {
        const u32 last_seq = c->recv_queue.arr[0].seq;
        const u32 min_seq = last_seq >= 34 ? (last_seq - 34) : (MAX_SEQUENCE - (34 - last_seq));

        while( !net_packet_queue_empty( &c->recv_queue ) && !sequence_more_recent( c->recv_queue.arr[c->recv_queue.tail].seq, min_seq ) )
            c->recv_queue.count--;
    }

    while( !net_packet_queue_empty( &c->ackd_queue ) && c->ackd_queue.arr[c->ackd_queue.tail].time > (2.f - M_EPS) )
        c->ackd_queue.count--;

    while( !net_packet_queue_empty( &c->pending_acks ) && c->pending_acks.arr[c->pending_acks.tail].time > (1.f + M_EPS) ) {
        c->pending_acks.count--;
        c->lost_packets++;
        printf( "Lost packet\n" );
    }
}

static void nc_packet_sent( connection_t *c, u32 size ) {
    if( !c ) return;

    if( net_packet_queue_exists( &c->sent_queue, c->seq_local ) ) {
        log_info( "Local sequence %d exists\n", c->seq_local );
        net_packet_queue_print( &c->sent_queue );
        return;
    }
    
    if( net_packet_queue_exists( &c->pending_acks, c->seq_local ) )
        return;

    // add a new sent packet info
    net_packet_info pi;
    pi.seq = c->seq_local;
    pi.time = 0.f;
    pi.size = size;

    net_packet_queue_insert( &c->sent_queue, &pi );
    net_packet_queue_insert( &c->pending_acks, &pi );

    if( c->mode == Server ) 
        printf( "Server sent packet with seq %d\n", pi.seq );
    else
        printf( "Client sent packet with seq %d\n", pi.seq );

    c->sent_packets++;
    c->seq_local++;
    if( c->seq_local > MAX_SEQUENCE )
        c->seq_local = 0;


}

static void nc_packet_received( connection_t *c, u32 seq, u32 size ) {
    if( !c ) return;

    c->recv_packets++;
    if( net_packet_queue_exists( &c->recv_queue, seq ) ) 
        return;

    // add new received packet info
    net_packet_info pi;
    pi.seq = seq;
    pi.time = 0.f;
    pi.size = size;

    net_packet_queue_insert( &c->recv_queue, &pi );

    if( c->mode == Server ) 
        printf( "Server received packet with seq %d\n", pi.seq );
    else
        printf( "Client received packet with seq %d\n", pi.seq );

    // update remote sequence if necessary
    if( sequence_more_recent( seq, c->seq_remote ) )
        c->seq_remote = seq;
}

static void nc_write_header( u8 *packet, u32 seq, u32 ack ) {
    if( !packet ) return;

    u32_to_bytes( protocol_id, packet );
    u32_to_bytes( seq,         packet + 4 );
    u32_to_bytes( ack,         packet + 8 );
}

static void nc_read_header( const u8 *packet, u32 *seq, u32 *ack ) {
    if( !packet || !seq || !ack ) return;

    bytes_to_u32( packet + 4, seq );
    bytes_to_u32( packet + 8, ack );
}

static int bit_index_for_seq( u32 seq, u32 ack ) {
    if( seq > ack )
        return ack + ( MAX_SEQUENCE - seq );
    else
        return ack - 1 - seq;
}

static u32 nc_get_ackbits( connection_t *c ) {
    if( !c ) return 0;

    u32 ack_bits = 0;
    const u32 ack = c->seq_remote;

    for( u32 i = c->recv_queue.tail; i >= 0; --i ) {
        if( c->recv_queue.arr[i].seq == ack || sequence_more_recent( c->recv_queue.arr[i].seq, ack ) )
            break;

        int bit_index = bit_index_for_seq( c->recv_queue.arr[i].seq, ack );

        if( bit_index <= 31 )
            ack_bits |= 1 << bit_index;
    }
    return ack_bits;
}

static void nc_process_ackbits( connection_t *c, u32 ack, u32 ack_bits ) {
    if( !c ) return; 
    if( net_packet_queue_empty( &c->pending_acks ) ) return;

    u32 i = c->pending_acks.tail;
    while( i >= 0 ) {
        bool acked = false;

        if( c->pending_acks.arr[i].seq == ack )
            acked = true;
        else if( !sequence_more_recent( c->pending_acks.arr[i].seq, ack ) ) {
            int bit_index = bit_index_for_seq( c->pending_acks.arr[i].seq, ack );
            if( bit_index <= 31 )
                acked = ( ack_bits >> bit_index ) & 1;
        }

        if( acked ) {
            c->rtt += ( c->pending_acks.arr[i].time - c->rtt ) * 0.1f;

            net_packet_queue_insert( &c->ackd_queue, c->pending_acks.list[i] );
            c->ackd_packets++;
        } else
            ++i;
    }
}

bool net_connection_send( connection_t *c, const u8 *packet, u32 size ) {
    if( !c || !packet ) return false;

    // header : protocol id + sequence + ack + ack_bits
    const int header = 12;
    u8 msg[size+header];

    check( c->running, "Connection not running\n" );
    
    u32 address;
    bytes_to_u32( c->address.ip, &address );
    check( address != 0, "Invalid destination address\n" );


    // write packet header
    nc_write_header( msg, c->seq_local, c->seq_remote );
    
    // add data 
    memcpy( &msg[header], packet, size );

    // send the message
    if( !net_send_packet( c->socket, &c->address, msg, size + header ) )
        return false;

    // update connection
    nc_packet_sent( c, size );

    return true;

error:
    return false;
}

int  net_connection_receive( connection_t *c, u8 *packet, u32 size ) {
    if( !c || !packet ) return -1;

    // header : protocol id + sequence + ack + ack_bits
    const int header = 12;

    u8 msg[size+header];
    net_addr sender;

    check( c->running, "Connection not running\n" );
    check( size > header, "Receiving buffer too small\n" );


    int bytes_read = net_receive_packet( c->socket, &sender, msg, size + header );
    if( bytes_read <= header )
        return 0;

    u32 id;
    bytes_to_u32( msg, &id );

    // bad header protocol
    if( id != protocol_id )
        return 0;

    // get header
    u32 seq, ack;
    nc_read_header( msg, &seq, &ack );

    // update connection
    nc_packet_received( c, seq, bytes_read - header );

    // client connecting to server
    if( c->mode == Server ) {
        log_info( "Server accepting connection from %d.%d.%d.%d:%d\n",
                sender.ip[0], sender.ip[1], sender.ip[2], sender.ip[3], sender.port ); 
        c->state = Connected;
        memcpy( &c->address, &sender, sizeof(net_addr) );
    } 

    if( net_addr_equal( &c->address, &sender ) ) {
        // Client connecting, finalize connection
        if( c->mode == Client && c->state == Connecting ) {
            log_info( "Client connected with server\n" );
            c->state = Connected;
        }

        // timeout reset
        c->timeout_accum = 0.f;

        // get packet data
        memcpy( packet, &msg[header], bytes_read - header );
        return bytes_read - header;
    }

error:
    return -1;
}


////////////////////////////////////////////////////////////////////////////
//                      SOCKETS

bool net_open_socket( net_socket *s, u16 port ) {
    check( !*s, "Socket already opened." );

    *s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    check( *s, "Failed to create socket.\n" );

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );

    check( !bind( *s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in) ),
            "Failed to bind socket.\n" );

    // set non-blocking IO
#ifdef BYTE_WIN32
    DWORD non_block = 1;
    check( ioctlsocket( *s, FIONBIO, &non_block ),
#else
    int non_block = 1;
    check( !fcntl( *s, F_SETFL, O_NONBLOCK, non_block ),
#endif
            "Failed to set non-blocking socket\n" );

    return true;

error:
    net_close_socket( s );
    return false;
}

void net_close_socket( net_socket *s ) {
    if( *s ) {
        close( *s );
        *s = 0;
    }
}

bool net_send_packet( net_socket s, const net_addr *dest, const void *packet, u32 packet_size ) {
    check( s, "Socket not opened\n" );
    check( packet, "Packet to send not allocated\n" );
    check( dest, "Packet destination address not allocated\n" );

    u32 addr;
    bytes_to_u32( dest->ip, &addr );

    struct sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl( addr );
    to.sin_port = htons( dest->port );

    int sent_bytes = sendto( s, (const char*)packet, packet_size, 0, (struct sockaddr*)&to, sizeof(struct sockaddr_in) );

    return sent_bytes == packet_size;

error:
    return false;
}

int  net_receive_packet( net_socket s, net_addr *sender, void *packet, u32 packet_size ) {
    check( s, "Socket not opened\n" );
    check( packet, "Receiving packet not allocated\n" );
    check( sender, "Sender address not allocated\n" );

    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);

    int received_bytes = recvfrom( s, (char*)packet, packet_size, 0, (struct sockaddr*)&from, &from_len );

    if( 0 >= received_bytes ) return 0;

    u32 addr = ntohl( from.sin_addr.s_addr );
    u16 port = ntohs( from.sin_port );

    net_addr_fill_int( sender, addr, port );


    return received_bytes;
    
error :
    return 0;
}

