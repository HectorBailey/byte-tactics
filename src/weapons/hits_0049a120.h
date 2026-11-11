// Hits_0049a120: what one explosion has already hit, so nothing takes damage
// twice. weapons.cpp and weapons_49a120.cpp each carried this same view; the
// name is still a placeholder (no evidence for a real one in data/thaldren/).
//
// Include it after the file's own Unit_0049a120 and Cell views: the struct
// holds pointers to both, and each file's view of them is its own.
struct Hits_0049a120 {
    int numUnits;
    int numFeatures;
    Unit_0049a120* units[20];
    Cell* features[64];

    // Each returns 0 if the thing was hit already, else records it (while
    // there is room) and returns 1.
    int AddUnit(Unit_0049a120* unit)
    {
        for (int k = 0; k < numUnits; k++) {
            if (units[k] == unit)
                return 0;
        }
        if (numUnits < 20) {
            units[numUnits] = unit;
            numUnits++;
        }
        return 1;
    }
    int AddFeature(Cell* cell)
    {
        for (int k = 0; k < numFeatures; k++) {
            if (features[k] == cell)
                return 0;
        }
        if (numFeatures < 64) {
            features[numFeatures] = cell;
            numFeatures++;
        }
        return 1;
    }
};
