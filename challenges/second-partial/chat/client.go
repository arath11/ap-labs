// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 227.

// Netcat is a simple read/write client for TCP servers.
package main

import (
	//	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

/*//entrada
//go run client.go -user user1 -server localhost:9000

//flag for user
var usuarioFlag = flag.String("user", "anonymousUser", "User's name")

//flag for the server
var servidorFlag = flag.String("server", "localhost:9000", "Client's conecttion to the ip port")
*/
//!+
func main() {
	args := os.Args

	servidor := "localhost:8000"

	usuario := "UsuarioGenerico"

	//entrada
	//go run client.go -user user1 -server localhost:9000
	//s1
	if args[3] == "-server" && args[1] == "-user" {
		servidor = args[4]
		usuario = args[2]
	} else {
		println("Error en los argumentos")
	}
	//conectar con tcp
	//conn, err := net.Dial("tcp", "localhost:9000")
	conn, err := net.Dial("tcp", servidor)

	if err != nil {
		log.Fatal(err)
	}
	//enviar usuario
	fmt.Fprint(conn, usuario)

	//intento de hacer manejo de usuario 2
	/*var setUserComm = fmt.Sprintf("/nuevoUsuario %s\n", usuario)

	if _, err := io.WriteString(conn, setUserComm); err != nil {
		log.Fatal(err)
	}*/

	done := make(chan struct{})
	go func() {
		io.Copy(os.Stdout, conn) // NOTE: ignoring errors
		log.Println("done")
		done <- struct{}{} // signal the main goroutine
	}()
	mustCopy(conn, os.Stdin)
	conn.Close()
	<-done // wait for background goroutine to finish

}

//!-

func mustCopy(dst io.Writer, src io.Reader) {
	if _, err := io.Copy(dst, src); err != nil {
		log.Fatal(err)
	}
}
