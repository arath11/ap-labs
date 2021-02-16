/*Julio Rosales */
package main


import (
	"fmt"
	"os"
)


func main() {
	if(1==len(os.Args)){
	  fmt.Println("Error, a name must be provided!  ")
	}else if(2<=len(os.Args)){
	  fmt.Print("Hello ")
	  for i :=1 ; i < len(os.Args); i++ {
	      if(i==len(os.Args)-1){
	       fmt.Print(os.Args[i]+", ") 
	      }else{
	        fmt.Print(os.Args[i]+" ") 
	      }
	  }
	  fmt.Print("Welcome to the jungle.")
	  fmt.Println("")
	}
}