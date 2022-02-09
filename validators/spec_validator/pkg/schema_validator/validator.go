package schemavalidator

import (
	"encoding/json"
	"fmt"
	"io/fs"
	"io/ioutil"
	"log"
	"os"
	"path"
	"path/filepath"
	"strings"

	js "github.com/santhosh-tekuri/jsonschema/v5"
)

type SchemaValidator struct {
	s *js.Schema
}

func New(schemaPath string) (*SchemaValidator, error) {
	if _, err := os.Lstat(schemaPath); err != nil {
		return nil, fmt.Errorf("no such file: %s", schemaPath)
	}

	c := js.NewCompiler()
	schemaDir := path.Dir(schemaPath)

	// register all available extensions
	err := filepath.WalkDir(path.Join(schemaDir, "extensions"), func(fpath string, d fs.DirEntry, err error) error {
		if !d.IsDir() && filepath.Ext(fpath) == ".json" {
			f, err := os.Open(fpath)
			if err != nil {
				return fmt.Errorf("could not open extension spec: %v", err)
			}
			defer f.Close()

			rel, err := filepath.Rel(schemaDir, fpath)
			if err != nil {
				// should never happen
				panic(err)
			}

			url := path.Join("/", strings.TrimSuffix(rel, filepath.Ext(rel)))
			if err := c.AddResource(url, f); err != nil {
				return err
			}

			log.Printf("Registered ext schema %v -> %v\n", url, fpath)
		}

		return nil
	})
	if err != nil {
		return nil, err
	}

	s, err := c.Compile(schemaPath)
	if err != nil {
		return nil, err
	}

	return &SchemaValidator{s}, nil
}

func (sv *SchemaValidator) ValidateBytes(input []byte) error {
	var obj interface{}
	if err := json.Unmarshal(input, &obj); err != nil {
		return fmt.Errorf("unable to unmarshal json: %v", err)
	}

	return sv.s.Validate(obj)
}

func (sv *SchemaValidator) ValidateFile(filename string) error {
	data, err := ioutil.ReadFile(filename)
	if err != nil {
		return fmt.Errorf("cannot open input file: %v", err)
	}

	return sv.ValidateBytes(data)
}
