package schemavalidator

import (
	"testing"

	js "github.com/santhosh-tekuri/jsonschema/v5"
	"github.com/stretchr/testify/require"
)

func TestSchemaVersion(t *testing.T) {
	json := `{
		"schemaVersion": {
			"major": 2,
			"minor": 0
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func xTestValidateBytesComplexRunArtifact(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"testRunStart": {
				"name": "Error Monitor",
				"version": "392747070",
				"parameters": {
					"@type": "type.googleapis.com/meltan.error_monitor.Params",
					"pollingIntervalSecs": 1,
					"runtimeSecs": 10,
					"ceccThreshold": {
						"maxCountPerDay": 4000
					},
					"ueccThreshold": {
						"maxCountPerDay": 0
					},
					"dimmNameMap": {},
					"monitors": [
						"PCIE_ERROR_MONITOR"
					],
					"pcicrawlerPath": "",
					"pcieThresholds": []
				},
				"dutInfo": [
					{
						"hostname": "yv3-slot2",
						"hardwareComponents":
						[
							{
								"hardwareInfoId": "0",
								"arena": "",
								"name": "PCIE_NODE:0000:04:00.0",
								"partNumber": "",
								"manufacturer": "",
								"mfgPartNumber": "",
								"partType": "endpoint",
								"componentLocation": {
									"devpath": "",
									"odataId": "",
									"blockpath": "0",
									"serialNumber": ""
								}
							},
							{
								"hardwareInfoId": "1",
								"arena": "",
								"name": "PCIE_NODE:0000:00:1d.0",
								"partNumber": "",
								"manufacturer": "",
								"mfgPartNumber": "",
								"partType": "root_port",
								"componentLocation":
								{
									"devpath": "",
									"odataId": "",
									"blockpath": "1",
									"serialNumber": ""
								}
							}
						],
						"softwareInfos":
						[]
					}
				]
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-09-17T09:24:17.113215889Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestValidateBytesInvalid(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"log": {
				"text": "log text",
				"severity": "INFO"
			}
		},
		"testStepArtifact": {
			"testStepId": "1",
			"log": {
				"text": "log text",
				"severity": "INFO"
			}
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	var ve *js.ValidationError
	require.ErrorAs(t, err, &ve)
}

func TestRunLogSimple(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"log": {
				"message": "log text",
				"severity": "INFO"
			}
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepLogSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepId": "1",
			"log": {
				"message": "log text",
				"severity": "INFO"
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestRunErrorSimple(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"error": {
				"symptom": "test-symptom"
			}
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepErrorSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepId": "1",
			"error": {
				"message": "log text",
				"symptom": "test-symptom",
				"softwareInfoIds": [
					"1"
				]
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepFileSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepId": "1",
			"file": {
				"displayName": "mem_cfg_log",
				"uri": "file:///root/mem_cfg_log",
				"description": "DIMM configuration settings.",
				"contentType": "text/plain",
				"isSnapshot": false
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepDiagnosisSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"diagnosis": {
				"verdict": "mlc-intranode-bandwidth-pass",
				"type": "PASS",
				"message": "intranode bandwidth within threshold.",
				"hardwareInfoId": "1",
				"subcomponent": {
					"type": "BUS",
					"name": "QPI1",
					"location": "CPU-3-2-3",
					"version": "1",
					"revision": "0"
				}
			},
			"testStepId": "1"
		},
		"sequenceNumber": 1,
		"timestamp": "2022-07-25T02:15:58.296032Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepExtensionSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"extension": {
				"name": "extension example",
				"content": {
					"@type": "ExampleExtension",
					"stringField": "example string",
					"numberField": "42"
				}
			},
			"testStepId": "2321"
		},
		"sequenceNumber": 600,
		"timestamp": "2022-07-26T02:34:06.249683Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func validateString(t *testing.T, json string) error {
	const schema string = "../../../../json_spec/output/spec.json"

	sv, err := New(schema)
	require.NoError(t, err)

	err = sv.ValidateBytes([]byte(json))
	if err != nil {
		t.Logf("error in validation: %#v", err)
	}

	return err
}
