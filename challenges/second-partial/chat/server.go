// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 254.
//!+

// Chat is a server that lets Users chat with each other.
package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"os"
	"strings"
	"time"
)

//!+broadcaster

type User struct {
	channel  chan<- string
	conn     net.Conn
	username string
	loggedAt string
	isBanned bool
	isAdmin  bool
	ipDir    string
}

type BroadcastMessage struct {
	ipDir   string
	message string
}

var (
	entering     = make(chan User)
	leaving      = make(chan User)
	broadcasterM = make(chan BroadcastMessage)
	broadcasterT = make(chan string)
)
var Users = make(map[string]*User)
var CantidadU = 0

func broadcaster() {
	for {

		select {

		case msg := <-broadcasterM:
			for _, u := range Users {
				if msg.ipDir != u.ipDir {
					u.channel <- Users[msg.ipDir].username + " > " + msg.message
				}
			}

		case msg := <-broadcasterT:
			for _, u := range Users {
				u.channel <- "irc-server > [" + msg + "] was kicked"
			}

		//todo
		case cli := <-entering:
			for _, u := range Users {
				if cli.ipDir != u.ipDir {
					u.channel <- "irc-server > New connected user [" + Users[cli.ipDir].username + "]"
				} else {
					u.channel <- "irc-server > Welcome to the Simple IRC Server"
					u.channel <- "irc-server > Your user [" + Users[cli.ipDir].username + "] is successfully logged"
					fmt.Println("irc-server > New connected user [" + Users[cli.ipDir].username + "]")
				}
			}
		//todo
		case cli := <-leaving:

			if cli.isAdmin {

				CantidadU = CantidadU - 1
				//sacarlo de la lista
				tmpUser := cli.username

				delete(Users, cli.ipDir)
				close(cli.channel)
				cli.conn.Close()

				if CantidadU > 0 {

					//buscar nuevo admin cuando se salga el anterior solo si almenos existe un usuario
					for _, c := range Users {
						fmt.Println("Si entre aqui admin a cambiar ")

						//broadcasterT <- "irc-server > [" + tmpUser + "] left the channel"
						//tmp1 := "irc-server > [" + tmpUser + "] left the channel"

						//broadcasterC <- tmp1

						c.isAdmin = true

						c.channel <- "irc-server > You're the new IRC Server ADMIN"

						//broadcasterT <- "[" + c.username + "] was promoted as the channel ADMIN"

						for _, c2 := range Users {
							c2.channel <- "irc-server > [" + tmpUser + "] left the channel"
							c2.channel <- "irc-server > [" + c.username + "] was promoted as the channel ADMIN"
						}

						fmt.Println("[" + c.username + "] was promoted as the channel ADMIN")

						break
					}
				}

			} else if !cli.isBanned {
				CantidadU = CantidadU - 1

				for _, c := range Users {
					if cli.ipDir != c.ipDir {
						c.channel <- "irc-server > [" + Users[cli.ipDir].username + "] left the channel"
					}
				}

				close(cli.channel)
				cli.conn.Close()
				fmt.Println("irc-server > [" + Users[cli.ipDir].username + "] left")
			} else {
				delete(Users, cli.ipDir)
			}

		}
	}
}

//!-broadcaster

//!+handleConn
func handleConn(conn net.Conn) {
	ch := make(chan string) // outgoing User messages
	who := conn.RemoteAddr().String()

	// read username from connection
	buff := make([]byte, 128)
	conn.Read(buff)
	name := string(strings.Trim(string(buff), "\x00"))

	utc := time.Now().UTC()
	local := utc
	location, err := time.LoadLocation("America/Mexico_City")
	if err == nil {
		local = local.In(location)
	}

	// initialize User
	if CantidadU == 0 {
		//listo
		//Users[who].channel <- "irc-server > Local time: " + local.Location().String() + " " + local.Format("15:04")
		//todo poner esta fecha en el usuario
		//Users[who].channel <- "irc-server > Local time: " + local.Location().String() + " " + local.Format("2006-01-02 15:04:05")

		Users[who] = &User{
			channel:  ch,
			conn:     conn,
			ipDir:    who,
			username: name,
			loggedAt: strings.Replace(local.Format("2006-01-02 15:04:05"), "T", " ", 1),
			isBanned: false,
			isAdmin:  true,
		}
		CantidadU = 1
		// start User
		go UserWriter(conn, ch, Users[who].username)

		//avisar
		//server
		fmt.Println("irc-server > New connected user [" + Users[who].username + "]")
		fmt.Println("irc-server > [" + Users[who].username + "] was promoted as the channel ADMIN")

		//to User
		Users[who].channel <- "irc-server > Welcome to the Simple IRC Server"
		Users[who].channel <- "irc-server > Your user [" + Users[who].username + "] is successfully logged"
		Users[who].channel <- "irc-server > Congrats, you were the first user."
		Users[who].channel <- "irc-server > You are the new IRC Server ADMIN"

	} else {

		Users[who] = &User{
			channel:  ch,
			conn:     conn,
			ipDir:    who,
			username: name,
			loggedAt: strings.Replace(local.Format("2006-01-02 15:04:05"), "T", " ", 1),
			isBanned: false,
			isAdmin:  false,
		}
		CantidadU++

		// start User
		go UserWriter(conn, ch, Users[who].username)

		//avisar
		entering <- *Users[who]
	}

	// continously scan User input
	input := bufio.NewScanner(conn)
	for input.Scan() {
		entrada := strings.Fields(input.Text())

		if len(entrada) > 0 {
			switch entrada[0] {
			case "/time":
				//listo
				utc := time.Now().UTC()
				local := utc
				location, err := time.LoadLocation("America/Mexico_City")
				if err == nil {
					local = local.In(location)
				}
				Users[who].channel <- "irc-server > Local time: " + local.Location().String() + " " + local.Format("15:04")
				//todo poner esta fecha en el usuario
				//Users[who].channel <- "irc-server > Local time: " + local.Location().String() + " " + local.Format("2006-01-02 15:04:05")
				//1
			case "/users":
				//listo
				if len(entrada) == 1 {
					for _, c := range Users {
						Users[who].channel <- "irc-server > " + Users[c.ipDir].username + " - connected since " + Users[c.ipDir].loggedAt
					}
				}
			case "/user":
				//listo
				if len(entrada) == 2 {
					userExits := false
					for _, c := range Users {
						//comparar que si exista el que recibe
						//la neta debi usar un hash jajja seria mejor
						if c.username == entrada[1] {
							userExits = true
							Users[who].channel <- "irc-server > username: " + c.username + ", ipDir: " + c.ipDir + ", Connected since: " + c.loggedAt
						}
					}
					if !userExits {
						Users[who].channel <- "irc-server > user [" + entrada[1] + "] is not connected to the server"
					} else {
						fmt.Fprint(conn, Users[who].username+" > ")
					}

				} else {
					Users[who].channel <- "irc-server > error with /users to many arguments"
				}

			case "/msg":
				//listo
				if len(entrada) >= 3 {
					enviado := false
					for _, c := range Users {
						//comparar que si exista el que recibe
						if c.username == entrada[1] && Users[who].username != entrada[1] {
							enviado = true

							c.channel <- Users[who].username + " > " + strings.Join(entrada[2:], " ")
							//c.channel <- Users[who].username + " > " + entrada[2]strings.Join(entrada[2:], " ")
						}
					}

					//decirle que no se pudo
					if !enviado {
						Users[who].channel <- "irc-server > user [" + entrada[1] + "] is not connected to the server"
					} else {
						fmt.Fprint(conn, Users[who].username+" > ")
					}

				} else {
					Users[who].channel <- "irc-server > Error Usage: /msg <user> <msg>"
				}
			case "/kick":
				//todo
				if len(entrada) == 2 {
					doesUserExist := false
					if Users[who].isAdmin == true && entrada[1] != Users[who].username {
						//llamar

						for _, c := range Users {
							if c.username == entrada[1] {
								//lo  encontro
								//a sacarlo
								fmt.Fprint(c.conn, "\rirc-server > You're kicked from this channel")

								fmt.Println("irc-server > [" + entrada[1] + "] was kicked")
								//avisar a todos

								c.isBanned = true
								//delete(Users, c.ipDir)
								close(c.channel)
								c.conn.Close()

								broadcasterT <- entrada[1]
								//falta anunciar a todos

								doesUserExist = true
								break
							}
						}
						if doesUserExist {
							fmt.Fprint(conn, Users[who].username+" > ")
						} else {
							Users[who].channel <- "irc-server > user [" + entrada[1] + "] is not connected to the server"
						}

					} else {
						if entrada[1] == Users[who].username {
							Users[who].channel <- "irc-server > Error You cannot kick yourself"
						} else {
							Users[who].channel <- "irc-server > Error You are not an admin"
						}
					}
				} else {
					Users[who].channel <- "irc-server > Error With /kick <user>"
				}

			default:
				//todo
				//sin broadcaster
				/*
					for _, c := range Users {
						//comparar que si exista el que recibe
						if c.username != Users[who].username {
							c.channel <- Users[who].username + " > " + input.Text()
						}
					}*/
				//con  broadcaster pero requiere de una struct extra
				broadcasterM <- BroadcastMessage{
					ipDir:   who,
					message: input.Text(),
				}
				fmt.Fprint(conn, Users[who].username+" > ")
			}
		}
		//fmt.Fprint(conn, Users[who].username+" > ")
	}

	leaving <- *Users[who]
}

func UserWriter(conn net.Conn, ch <-chan string, user string) {
	for msg := range ch {
		fmt.Fprint(conn, "\r"+msg+"\n"+user+" > ")
	}
}

//!-handleConn

//!+main
func main() {

	args := os.Args
	//go run server.go -host localhost -port 9000

	hostFlag := "localhost"
	portFlag := "8000"

	if len(args) == 5 {
		if args[1] == "-host" && args[3] == "-port" {
			hostFlag = args[2]
			portFlag = args[4]
			//original
			//listener, err := net.Listen("tcp", "localhost:8000")
			listener, err := net.Listen("tcp", hostFlag+":"+portFlag)

			//saludo del server
			fmt.Printf("irc-server > Simple IRC Server started at %s:%s\nirc-server > Ready for receiving new Users", hostFlag, portFlag)

			if err != nil {
				log.Fatal(err)
			}
			//println()

			go broadcaster()
			for {
				conn, err := listener.Accept()
				if err != nil {
					log.Print(err)
					continue
				}
				go handleConn(conn)
			}
		}
	} else {
		println("Error al iniciar el servidor, no se encontro puerto y host\n ")

	}
}
