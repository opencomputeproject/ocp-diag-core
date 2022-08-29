package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"os"
	"path"

	schemavalidator "github.com/opencomputeproject/test_and_validation/json_validator/pkg/schema_validator"
)

var (
	flagSet    *flag.FlagSet
	flagSchema *string
	flagJSONL  *bool
)

func initFlags() {
	cmd := os.Args[0]

	flagSet = flag.NewFlagSet(cmd, flag.ContinueOnError)
	flagSchema = flagSet.String("schema", "", "Path to schema file")
	flagJSONL = flagSet.Bool("jsonl", true, "Treat input data as a JSONL. Otherwise it's a whole JSON document")

	flagSet.Usage = func() {
		fmt.Fprintf(flagSet.Output(),
			`Usage:

  %s [flags] filename

  filename: path to file to validate, - for stdin

Flags:
`, path.Base(cmd))
		flagSet.PrintDefaults()
	}

	if err := flagSet.Parse(os.Args[1:]); err != nil {
		if err == flag.ErrHelp {
			return
		}
		log.Fatalf("failed to parse args: %v", err)
	}
}

func main() {
	initFlags()

	if *flagSchema == "" {
		log.Fatalln("The <schema> flag is mandatory")
	}

	filename := flagSet.Arg(0)
	if filename == "" {
		log.Fatalln("no filename passed, see -help")
	}

	sv, err := schemavalidator.New(*flagSchema)
	if err != nil {
		log.Fatal(err)
	}

	if *flagJSONL {
		if err := validateJSONL(sv, filename); err != nil {
			log.Fatalf("JSONL validation failed: %#v", err)
		}
	} else {
		if err := validateJSON(sv, filename); err != nil {
			log.Fatalf("JSON validation failed: %#v", err)
		}
	}

	fmt.Println("all ok")
}

func validateJSON(sv *schemavalidator.SchemaValidator, filename string) error {
	return sv.ValidateFile(filename)
}

func validateJSONL(sv *schemavalidator.SchemaValidator, filename string) error {
	var file *os.File
	if filename == "-" {
		// no file
		file = os.Stdin
	} else {
		file, err := os.Open(filename)
		if err != nil {
			return err
		}
		defer file.Close()
	}

	bs := bufio.NewScanner(file)
	for bs.Scan() {
		line := bs.Text()
		if err := sv.ValidateBytes([]byte(line)); err != nil {
			log.Printf("failed on line: %s", line)
			return err
		}
	}

	return nil
}
