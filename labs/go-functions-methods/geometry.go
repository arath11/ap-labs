package main

import (
	"fmt"
	"math"
	"math/rand"
	"os"
	"strconv"
	"time"
)

type Point struct{ x, y float64 }

func (p Point) X() float64 {
	return p.x
}

func (p Point) Y() float64 {
	return p.y
}

func Distance(p, q Point) float64 {
	return math.Hypot(q.X()-p.X(), q.Y()-p.Y())
}

func (p Point) Distance(q Point) float64 {
	return math.Round(math.Hypot(q.X()-p.X(), q.Y()-p.Y())*100) / 100
}

type Path []Point

func (path Path) Distance() float64 {
	sum := 0.0
	for i := range path {
		if i > 0 {
			sum += path[i-1].Distance(path[i])
		}
	}
	return sum
}

func onSegment(a, b, c Point) bool {
	return (b.x <= math.Max(a.x, c.x) && b.x >= math.Min(a.x, c.x) &&
		b.y <= math.Max(a.y, c.y) && b.y >= math.Min(a.y, c.y))
}

func orientation(a, b, c Point) int {
	var val float64 = (b.y-a.y)*(c.x-b.x) - (b.x-a.x)*(c.y-b.y)

	if val == 0 {
		return 0 // colinear
	}
	if val < 0 {
		return 2 //counter clockwise
	} else {
		return 1 // clockvise
	}
}

//return true if they intersect
func doIntersect(p1, q1, p2, q2 Point) bool {

	o1 := orientation(p1, q1, p2)
	o2 := orientation(p1, q1, q2)
	o3 := orientation(p2, q2, p1)
	o4 := orientation(p2, q2, q1)

	//caso general

	if (o1 != o2) && (o3 != o4) {
		return true
	}

	//colineales
	if (o1 == 0) && (onSegment(p1, p2, q1)) {
		return true
	} else if (o2 == 0) && (onSegment(p1, q2, q1)) {
		return true
	} else if (o3 == 0) && (onSegment(p2, p1, q2)) {
		return true
	} else if (o4 == 0) && (onSegment(p2, q1, q2)) {
		return true
	}

	return false
}

func (tmp Path) verificar() bool {
	var seIntersectan bool

	for i := 0; i < len(tmp)-3 && !seIntersectan; i++ {

		seIntersectan = doIntersect(tmp[i], tmp[i+1], tmp[i+2], tmp[i+3])

	}

	return seIntersectan
}

func main() {
	path1 := Path{}

	if len(os.Args) == 2 {
		if sides, err := strconv.Atoi(os.Args[1]); err == nil {
			if sides > 2 {

				fmt.Printf("- Generating a [%d] sides figure\n", sides)
				for i := 0; i < sides; i++ {
					inicioRandom := rand.NewSource(time.Now().UnixNano())
					random := rand.New(inicioRandom)

					path1 = append(path1, Point{random.Float64()*100 - (-100) + (-100), random.Float64()*100 - (-100) + (-100)})

					//verificar
					for path1.verificar() {
						//hara uno nuevo si encuentra problemas
						path1[i] = Point{random.Float64()*100 - (-100) + (-100), random.Float64()*100 - (-100) + (-100)}
					}
				}

				//imprimir

				fmt.Printf("- Figure's vertices\n")
				for i := range path1 {
					fmt.Printf("   ( %.2f, %.2f)\n", path1[i].X(), path1[i].Y())
				}

				fmt.Println("- Figure's Perimeter")
				fmt.Printf("- ")

				for i := 0; i < sides; i++ {
					if i == 0 {
						fmt.Printf("   %.2f+", path1[i].Distance(path1[(i+1)%sides]))
					} else {
						fmt.Printf("  %.2f  +", path1[i].Distance(path1[(i+1)%sides]))
					}
				}
				fmt.Printf("%.2f  ", path1[len(path1)-2].Distance(path1[len(path1)-1]))
				//fmt.Printf("%.2f = %.2f\n", path[len(path)-2].Distance(path[len(path)-1]), path.Distance())

				//prueba2
				fmt.Printf(" = %.2f\n", path1.Distance()+path1[sides-1].Distance(path1[0]))

			} else {
				fmt.Printf("No se puede generar, necesita más lados \n")
			}
		} else {
			fmt.Printf("Error\n")
		}
	} else {
		fmt.Printf("Error\n")
	}
}
