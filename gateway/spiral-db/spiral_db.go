
// ═══════════════════════════════════════════
// PRESERVATION HANDLERS
// ═══════════════════════════════════════════
func preserveHandler(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case "POST":
		// Trigger full preservation
		go PreserveAll()
		json.NewEncoder(w).Encode(map[string]interface{}{
			"status": "preservation_started",
			"message": "All projects being preserved with FHE encryption",
		})
	case "GET":
		// List preserved artifacts
		artifactType := r.URL.Query().Get("type")
		if artifactType == "" { artifactType = "phi-sig" }
		results, _ := queryPreserved(artifactType)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"type": artifactType,
			"count": len(results),
			"artifacts": results,
		})
	}
}

// Add to main():
// http.HandleFunc("/preserve", preserveHandler)
// http.HandleFunc("/preserve/", preserveHandler)
