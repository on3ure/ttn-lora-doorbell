#!/usr/bin/perl

use v5.10;
use strict;
use utf8;
use Mojolicious::Lite;
use Config::YAML;
use Data::Dumper;
use WWW::PushBullet;
use Mojo::UserAgent;

# YAML based config
my $config = Config::YAML->new( config => "config.yaml" );

# Mojo config
my $mojoconfig = plugin Config => { file => 'mojo.conf' };

# PushBullet Config
my $pb = WWW::PushBullet->new( { apikey => $config->{pb} } );

# UserAgent
my $ua  = Mojo::UserAgent->new;

# log add papertrail later
helper log => sub {
    my $self = shift;
    my $data = shift;

    app->log->info($data);
    return;
};

# securityheaders
helper securityheaders => sub {
    my $self = shift;
    foreach my $key ( keys %{ $config->{'securityheaders'} } ) {
        $self->res->headers->append(
            $key => $config->{'securityheaders'}->{$key} );
    }
    return;
};

# token authentication
helper auth => sub {
    my $self = shift;
    if ( !defined( $config->{tokens}->{ $self->stash('token') } ) ) {
        app->log->error( 'authentication denied by token ['
              . $self->stash('token')
              . '] token unknown' );

        $self->render(
            json => {
                'errorcode' => '-1',
                'message'   => 'wrong token',
                'status'    => 'ERROR'
            }
        );

        return;
    }

    my $appuser = $config->{tokens}->{ $self->stash('token') };
    app->log->info( '['
          . __LINE__ . '] ['
          . $appuser
          . '] authenticated by token ['
          . $self->stash('token')
          . ']' )
      if $config->{debug} eq 'true';
    app->log->info( '['
          . __LINE__ . '] ['
          . $appuser
          . '] client info ['
          . $self->whois
          . ']' )
      if $config->{debug} eq 'true';

    return $appuser;
};

get '/healthz' => sub {
    my $self = shift;
    app->log->info('[liveness probe]')
      if $config->{debug} eq 'true';

    return $self->render( json => 'Healty', status => '200' );
};

my $example_data = {
    'counter'        => 3,
    'payload_fields' => {
        'battery'     => 4324,
        'temperature' => '26.88',
        'light'       => 16,
        'event'       => 'button'
    },
    'dev_id'      => 'deurbel_lombardsijde',
    'app_id'      => 'deurbel_app',
    'payload_raw' => 'EOQAEAqA',
    'downlink_url' =>
'https://integrations.thethingsnetwork.org/ttn-eu/api/v2/down/deurbel_app/on3ure-deurbel?key=ttn-account-v2.N6_XF0mY4lS8VNIjn4H6onmzlD21MngrYG5frP5BMMw',
    'hardware_serial' => '0004A30B001FB98C',
    'metadata'        => {
        'time'       => '2019-04-19T00:18:07.611957659Z',
        'modulation' => 'LORA',
        'gateways'   => [
            {
                'time'   => '2019-04-19T00:18:07Z',
                'snr'    => 9,
                'gtw_id' => 'on3ure-lombardsijde',
                'gtw_trusted' =>
                  bless( do { \( my $o = 1 ) }, 'JSON::PP::Boolean' ),
                'rssi'      => -53,
                'latitude'  => '51.152122',
                'channel'   => 4,
                'longitude' => '2.765444',
                'timestamp' => 223154171,
                'rf_chain'  => 0
            }
        ],
        'frequency'   => '867.3',
        'data_rate'   => 'SF7BW125',
        'coding_rate' => '4/5'
    },
    'port' => 4
};

post '/ttn/:token' => sub {
    my $self = shift;

    # reder when we are done
    $self->render_later;

    # add securityheaders
    $self->securityheaders;

    # authenticate
    my $appuser = $self->auth;
    return unless $appuser;

    my $data = $self->req->json;

    #$self->log( Dumper $data);

    if ( $data->{payload_fields}->{event} eq 'interval' ) {
        $self->log( "Interval: "
              . $data->{dev_id}
              . " Battery: "
              . $data->{payload_fields}->{battery} );
    }
    if ( $data->{payload_fields}->{event} eq 'button' ) {
        $self->log( "Button Pressed: "
              . $data->{dev_id}
              . " Battery: "
              . $data->{payload_fields}->{battery} );
        my $body = $data->{dev_id};
        $body =~ s/_/ /g;
        $body =~ s/([\w']+)/\u\L$1/g;

        $pb->push_note(
            {
                title => 'LORA Doorbell',
                body  => $body,
            }
        );
    }

    $self->render(
        json => {
            'add' => 'ok'
        }
    );
};

app->start;

