/*
 * XMLQuery.java
 * Copyright 2020-22 PDF Association, Inc. https://www.pdfa.org
 *
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR001119C0079.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material are those of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA). Approved for public release.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
 */
package gcxml;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.*;

/**
 * Handles Arlington TSV data. In particular it understands the PDF version
 * based predicates and can perform reduction on the "Type" and "Links" fields
 * appropriately for a specified PDF version. e.g. the "fn:SinceVersion" 
 * predicate statement will be removed if the current PDF version is less than 
 * the operand in the predicate. Furthermore, utility methods are provided that
 * can reduce the other TSV fields, should the "Type" field be reduced. e.g. if 
 * the "Type" field has an entry "array;fn:SinceVersion(1.5,dictionary);stream"
 * then other TSV fields that are expressed in the complex manner (meaning with 
 * SEMI-COLON separators) will have the corresponding middle element removed if 
 * for PDF 1.0 to 1.4 inclusive.
  */
public class TSVHandler {
    
    /**
     * Complex Arlington "Type" fields can be reduced due to predicates.
     * This reduction then needs to be mirrored across other TSV fields
     * so that the number of SEMI-COLON separated elements matches.
     */
    public class TypeListModifier {
        private String      output_types;
        private boolean[]   input_was_reduced; 

        /**
         * Constructor. Default is that the input and output are the same,
         * so nothing is reduced.
         * @param types the original Arlington "Types" field
         */
        public TypeListModifier(String types) {
            String[] arr = types.split(";"); // split complex type
            output_types = types;
            input_was_reduced = new boolean[arr.length];
            for (int i = 0; i < arr.length; i++) {
                input_was_reduced[i] = false;
            }
        }
        
        /**
         * Returns true if something has been reduced due to versioning
         * 
         * @return true if something has been reduced
         */
        public boolean somethingReduced() {
            for (int i = 0; i < input_was_reduced.length; i++) {
                if (input_was_reduced[i]) {
                    return true;
                }
            } 
            return false;
        }
        
        /**
         * At least one type got reduced so need to
         * reduce various other TSV fields accordingly
         * - Check for SEMI-COLON to determine if complex
         * - Split based on SEMI-COLIN
         * - Remove each element that has been reduced
         * - Stitch things back together with SEMI-COLONS
         * 
         * @param str some complex Arlington field
         * @return  correspondingly reduced (transformed) Arlington field
         */
        public String reduceCorresponding(String str) {
            String[] arr = str.split(";"); // split complex type
            if (arr.length != input_was_reduced.length) {
                System.out.println("Error: pre-reduction lengths did not match for '" + str + "'!");
                return str;
            }
            String out_str = "";
            int out_len = 0;
            for (int i = 0; i < input_was_reduced.length; i++) {
                if (!input_was_reduced[i]) {
                    if (out_str.isBlank()) {
                        out_str = arr[i];
                    }
                    else {
                        out_str = out_str + ";" + arr[i];
                    }
                    out_len = out_len + 1;
                }
            }
            // Reduce even further for special fields...
            if ("[TRUE]".equals(out_str)) {
                out_str = "TRUE";
            }
            else if ("[FALSE]".equals(out_str)) {
                out_str = "FALSE";
            }

            return out_str;
        } 
        
        /**
         * Returns the reduced Types list, accounting for the PDF version
         * @return the reduce Types list as a String
         */
        public String getReducedTypes() {
            return output_types;
        }
    }
    
    /**
     * The list of all valid supported PDF versions
     */
    public final static double[] pdf_version = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0};
    
    /**
     * The list of all Arlington predefined types that require a Link. Alphabetical.
     */
    private final static String[] linked_types = {"array", "dictionary", "name-tree", "number-tree", "stream"};
        
    /**
     * The path to the latest TSV file set (typically "tsv/latest")
     */
    private static String path_to_tsv_files = "";
    
    /**
     * Constructor. 
     */
    public TSVHandler(){
        path_to_tsv_files = System.getProperty("user.dir") + "/tsv/latest/";
    }

    /**
     * Creates TSV file sets for all the PDF versions, based on 'path_to_tsv_files'
     * Deletes all existing PDF version sub-folders and files!
     */
    public void createAllVersionsTSV() {
        for (double x : pdf_version) {
            deleteTSVset(x);
            createTSVset(x);
        }
    }

    /**
    * Returns true if parameter is an Arlington type that always requires a Link 
    * 
    * @param t a single Arlington type
    * 
    * @return true if and only if t is a type that always requires a Link
    */
   public boolean isLinkedType(String t) {
       for (String linked_type : linked_types) {
           if (linked_type.equals(t)) {
               return true;
           }
       } 
       return false;
   }

   /**
     * Creates a TSV file set for the specified PDF version
     * 
     * @param version  the PDF version between 1.0 to 2.0 inclusive
     */
    private void createTSVset(double version) {
        final String delimiter = "\t";
        
        File folder = new File(path_to_tsv_files);
        var list_of_files = folder.listFiles();
        
        for (File file : list_of_files){
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    BufferedReader tsv_reader;
                    var temp = path_to_tsv_files;
                    String file_name = file.getName().substring(0, file.getName().length()-4);
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    String[] row;
                    String output_string = "";
                    String entry = "";
                    System.out.println("================\nProcessing " + file_name + " for version " + version);
                    String current_line = tsv_reader.readLine();
                    // First line is header
                    if (current_line != null) {
                        output_string = current_line + "\n";
                    }
                    while ((current_line = tsv_reader.readLine()) != null) {
                        row = current_line.split(delimiter, -1);
                        if (row.length != 12) {
                            System.out.println("Error: " + file_name + " had " + row.length + " rows, not 12!\n");
                        } 
                        else {
                            // Field 0 = Key
                            String key_name = row[0];
                            
                            // Field 1 = Type: complex type, SEMI-COLON separated, may have version-based predicates
                            String data_type = row[1]; 
                            
                            // Field  2= SinceVersion: 1.0, 1.1, ..., 2.0 inclusive - may have predicates!
                            String since_version = row[2];
                            
                            // Field 3 = DeprecatedIn
                            String deprecated = row[3];
                            
                            // Field 4 = Required possibly wrapped in "fn:IsRequired(...)" with version-based predicates
                            String required = row[4];
                            
                            // Field 5 = IndirectReference: possibly complex so may need reduction
                            String indirect_ref = row[5];
                            
                            // Field 6 = IndirectReference: possibly complex so may need reduction
                            String inheritable = row[6];
                            
                            // Field 7 = DefaultValue: possibly complex so may need reduction
                            String default_value = row[7];
                            
                            // Field 8 = PossibleValues: possibly complex, may also have version-based predicates
                            String possible_values = row[8];
                            
                            // Field 9 = SpecialCase: possibly complex, may also have version-based predicates
                            String special_case = row[9];
                            
                            // Field 10 = Links: possibly complex, may also have version-based predicates
                            String links = row[10];
                            
                            // Field 11 = Notes. Text
                            String notes = row[11];
                            
                            var updated_since_ver = new StringBuilder("");
                            if (reduceSinceVersion(since_version, version, updated_since_ver) <= version) {
                                System.out.println("\tKept key: " + key_name);
                                assert(!updated_since_ver.toString().isBlank());
                                if (!since_version.equals(updated_since_ver.toString())) {
                                    System.out.println("\t\tPredicate = " + updated_since_ver);
                                    since_version = updated_since_ver.toString();
                                }
                                TypeListModifier types_reduced = reduceTypesForVersion(data_type, version);
                                if (types_reduced.somethingReduced()) {
                                    // At least one type got reduced so need to
                                    // reduce various other TSV fields accordingly
                                    // BEFORE they themselves are reduced
                                    indirect_ref = types_reduced.reduceCorresponding(indirect_ref);
                                    default_value = types_reduced.reduceCorresponding(default_value);
                                    possible_values = types_reduced.reduceCorresponding(possible_values);
                                    special_case = types_reduced.reduceCorresponding(special_case);
                                    links = types_reduced.reduceCorresponding(links);
                                }
                                String links_reduced = reduceComplexForVersion(links, version);
                                String pv_reduced = reduceComplexForVersion(possible_values, version);
                               
                                if (required.startsWith("fn:IsRequired(")) {
                                    required = reduceRequiredForVersion(required, version);                                    
                                }
                                
                                // Did we reduce links to effectively nothing for a single basic type?
                                if ("[]".equals(links_reduced)) {
                                    assert !isLinkedType(types_reduced.output_types) : "Reduced to [] for a Type requiring a Link!";
                                    links_reduced = "";
                                }

                                String record =
                                        key_name + delimiter +
                                        types_reduced.output_types + delimiter +
                                        since_version + delimiter +
                                        deprecated + delimiter +
                                        required + delimiter +
                                        indirect_ref + delimiter +
                                        inheritable + delimiter +
                                        default_value + delimiter +
                                        pv_reduced + delimiter +
                                        special_case + delimiter +
                                        links_reduced + delimiter +
                                        notes;
                                entry += record + "\n";
                            }
                            else {
                                System.out.println("\tDropped key: " + key_name);
                            }
                        }            
                    } // while
                    // Did we exclude the entire object??
                    if (!entry.isEmpty()) {
                        output_string += entry;
                        writeToFile(output_string, file_name, version);
                    }
                    else {
                        System.out.println("\tNot writing file " + file_name + " for version " + version);                        
                    }
                } 
                catch (FileNotFoundException ex) {
                    Logger.getLogger(TSVHandler.class.getName()).log(Level.SEVERE, null, ex);
                } 
                catch (IOException ex) {
                    Logger.getLogger(TSVHandler.class.getName()).log(Level.SEVERE, null, ex);
                } // catch
            } // if file OK
        } // for file
    }

    /**
     * Writes a new TSV file to a specified PDF version folder
     * 
     * @param output_string string the tab-separated multi-line string to write
     * @param file the filename (i.e. PDF object)
     * @param version the PDF version used to select the sub-folder
     */
    private void writeToFile(String output_string, String file, double version) {
        BufferedWriter out = null;
        
        String path = System.getProperty("user.dir") + "/tsv/";
        path += version + "/" + file +".tsv";

        try {
            FileWriter fstream = new FileWriter(path, false);
            out = new BufferedWriter(fstream);
            out.write(output_string);
        }
        catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
        }
        finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException ex) {
                    Logger.getLogger(TSVHandler.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }

    /**
     * Deletes an entire PDF version specific folder of all Arlington TSV files
     * 
     * @param ver  the PDF version (1.0, 1.1, ..., 2.0)
     */
    private void deleteTSVset(double ver) {        
        String path = "";
        path +=  System.getProperty("user.dir") + "/tsv/";
        path += ver + "/";
        
        File folder = new File(path);
        var list_of_files = folder.listFiles();
        
        for (File file : list_of_files) {
            file.delete();
        }
    }
    
    /**
     * Finds the matching closing bracket ")" for the first open bracket "(" 
     * in a string. 
     * e.g. (abc) = 4
     *      fn((a==b),(c || (d!=e))) = 24
     * 
     * @param s  the string
     * @return -1 if no matching bracket pair or IndexOf matching ")" in string
     */
    public int indexOfOuterCloseBracket(String s) {
        int nested = 0;
        int i = 0;
        
        for (char ch: s.toCharArray()) {
            if (ch == '('){
                nested++;
            }
            else if (ch == ')') {
                if (nested == 1) {
                    return i;
                }
                nested--;
            }
            i++;
        }
        return -1;
    }

    
    /**
     * Processes an atomic Arlington entry that might contain version
     * predicates and reduces it appropriately for the specified PDF version.
     *
     * Version-specific predicates ONLY that are supported:
     * - fn:SinceVersion(x.y,zzz): keep/strip if version &gt;= x.y else remove
     * - fn:BeforeVersion(x.y,zzz): keep/strip if x.y &lt; version else remove
     * - fn:Deprecated(x.y,zzz): strip if version &lt; x.y else keep as-is with predicate
     * - fn:IsPDFVersion(x.y,zzz): keep/strip if version == x.y else remove
     *
     * @param str     the atomic element from an Arlington TSV file
     * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
     * 
     * @return the version-reduced equivalent appropriate for the version
     */
   public String reduceAtomicForVersion(String str, double version) {
        if ((str.isBlank()) || (!str.contains("fn:"))) {
            return str;
        }

        String   tsv_ver;
        String   tsv_s = str;
        // +3 = length of PDF version string "x.y"
        // +1 = trailing COMMA as predicate operand separator before atomic element
        if (str.startsWith("fn:SinceVersion(")) {
            tsv_ver = str.substring(16, 16+3);
            if (version >= Double.parseDouble(tsv_ver)) {
                tsv_s = str.substring(16+3+1, str.length()-1);
            }
            else {
                tsv_s = "";
            }
        }
        else if (str.startsWith("fn:BeforeVersion(")) {
            tsv_ver = str.substring(17, 17+3);
            if (Double.parseDouble(tsv_ver) < version) {
                tsv_s = str.substring(17+3+1, str.length()-1);
            }
            else {
                tsv_s = str;
            }
        }
        else if (str.startsWith("fn:Deprecated(")) {
            tsv_ver = str.substring(14, 14+3);
            if (version < Double.parseDouble(tsv_ver)) {
                tsv_s = str.substring(14+3+1, str.length()-1);
            }
            else {
                tsv_s = str;
            }
        }
        else if (str.startsWith("fn:IsPDFVersion(")) {
            tsv_ver = str.substring(16, 16+3);
            if (version == Double.parseDouble(tsv_ver)) {
                tsv_s = str.substring(16+3+1, str.length()-1);
            }
            else {
                tsv_s = "";
            }
        }
        return tsv_s;
    }
    
    /**
     * Processes an Arlington Type field that might contain version
     * predicates and reduces it appropriately for the specified PDF version.
     *
     * @param str     the Type field from an Arlington TSV file
     * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
     *
     * @return a TypeListModifier object, summarizing what happened
     */
    public TypeListModifier reduceTypesForVersion(String str, double version) {
        TypeListModifier  obj = new TypeListModifier(str);
        
        if ((str.isBlank()) || (!str.contains("fn:"))) {
            return obj;
        }

        String[] arr = str.split(";"); // split complex type
        
        String   out_types = "";
        String   tsv_s;
        int      i = 0;
        for (String a : arr) {
            tsv_s = reduceAtomicForVersion(a, version);

            // Append to output if there was anything to keep
            if (!tsv_s.isBlank()) {
                if (out_types.isBlank()) {
                    out_types = tsv_s;
                } else {
                    out_types = out_types + ";" + tsv_s;
                }
            }
            else {
                obj.input_was_reduced[i] = true;
            }
            i++;
        }
        obj.output_types = out_types;
        return obj;
    }
    
    /**
     * Processes any complex Arlington field ([];[];[]) that might contain version
     * predicates and reduces it appropriately for the specified PDF version.
     *
     * @param str     the Link field from an Arlington TSV file
     * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
     * 
     * @return the version-reduced equivalent appropriate for the version
     */
    public String reduceComplexForVersion(String str, double version) {
        if ((str.isBlank()) || (!str.contains("fn:"))) {
            return str;
        }

        String[] arr = str.split(";"); // split complex type
        
        String   out_links = "";
        for (String a : arr) {
            // strip [ and ]    
            a = a.substring(1, a.length() - 1);
            
            String link = "";
            // COMMAs are ambiguous: separators or inside predicates?
            while (!a.isBlank()) {
                if (a.startsWith("fn:")) {
                    // get up to closing bracket )
                    int i = indexOfOuterCloseBracket(a);
                    assert i != -1: "No ')' for predicate!";
                    
                    // Get encapsulating predicate incl. close bracket
                    String s = a.substring(0, i + 1);

                    if (i + 2 < a.length()) {
                        a = a.substring(i + 2, a.length());
                    }
                    else {
                        a = "";
                    }
                    String reduced = reduceAtomicForVersion(s, version);

                    // Append to output if there was anything to keep
                    if (!reduced.isBlank()) {
                        if (link.isBlank()) {
                            link = reduced;
                        } else {
                            link = link + "," + reduced;
                        }
                    }
                    // remove COMMA if not at end of string
                    if ((!a.isBlank()) && (a.charAt(0) == ',')) {
                        a = a.substring(1, a.length());
                    }
                }
                else {
                    String s = a;
                    if (a.indexOf(',') >= 0) {
                        s = a.substring(0, a.indexOf(','));
                        a = a.substring(a.indexOf(',') + 1, a.length());
                    }
                    else {
                        a = "";
                    }
                    if (link.isBlank()) {
                        link = s;
                    } else {
                        link = link + "," + s;
                    }                    
                }
            } // while
            
            if (out_links.isBlank()) {
                out_links = "[" + link + "]";
            }
            else {
                out_links = out_links + ";[" + link + "]"; 
            }
        } // for
        return out_links;
    }
    
    
    /**
     * Processes an Arlington "Required" field that might contain version
     * predicates and reduces it appropriately for the specified PDF version.
     * Outer predicate is always "fn:IsRequired(...)". Examples include:
     * - fn:IsRequired(fn:SinceVersion(1.5))
     * - fn:IsRequired(fn:BeforeVersion(1.3) || fn:IsPresent(SomeKey))
     * - fn:IsRequired(fn:SinceVersion(2.0) || fn:AnotherPredicate(...))
     *
     * @param reqd     the Required field from an Arlington TSV file
     * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
     * 
     * @return the version-reduced equivalent appropriate for the version
     */
    public String reduceRequiredForVersion(String reqd, double version) {
        if (reqd.equals("TRUE") || reqd.equals("FALSE")) {
            return reqd;
        }
        
        assert (reqd.length() > 30) : "fnIsRequired() is too short!";
        String r = reqd.substring(14, reqd.length()-1);
        String tsv_ver;
        
        if (r.startsWith("fn:BeforeVersion(")) {
            tsv_ver = r.substring(17, 17+3);
            // Don't process complex expressions (e.g. with " && " or " || ")
            if ((version < Double.parseDouble(tsv_ver)) && (reqd.indexOf(' ') == -1)) {
                return "TRUE";
            }
        }                         
        else if (r.startsWith("fn:IsPDFVersion(")) {
            tsv_ver = r.substring(16, 16+3);
            // Don't process complex expressions (e.g. with " && " or " || ")
            if ((Double.parseDouble(tsv_ver) == version) && (reqd.indexOf(' ') == -1)) {
                return "TRUE";
            }
        }
        else if (r.startsWith("fn:SinceVersion(")) {
            tsv_ver = r.substring(16, 16+3);
            if (version >= Double.parseDouble(tsv_ver)) {
                if ((reqd.indexOf(' ') == -1) || (reqd.contains(" || "))) {
                    return "TRUE";
                }
            }
            else {
                r = reqd.replace("fn:SinceVersion("+tsv_ver+") || ", "");
                return r;
            }
        }
        return reqd;
    }
     
    /**
     * Processes an Arlington "SinceVersion" field that might contain version
     * predicates and reduces it appropriately for the specified PDF version.
     * Examples include:
     * - fn:Extension(XYZ)
     * - fn:Extension(XYZ,1.3)
     * - fn:Eval(fn:Extension(XYZ,1.5) || 2.0)
     *
     * @param sincever  the SinceVersion field from an Arlington TSV file
     * @param for_version the PDF version we are currently processing
     * @param reduced_sincever output string for the PDF version being created
     * 
     * @return the lowest PDF version ("1.0", "1.1", etc)
     */
    public double reduceSinceVersion(String sincever, double for_version, final StringBuilder reduced_sincever) {
        Double arl_extn_ver;
        
        assert(!sincever.isBlank()) : "never have an empty SinceVersion field";
                
        if (sincever.startsWith("fn:")) {
            if (sincever.startsWith("fn:Eval")) {
                // Predicate: fn:Eval(fn:Extension(AAA,x.y) || a.b) - extension AA since PDF x.y or part of core PDF since a.b
                Pattern p = Pattern.compile("fn:Eval\\(fn:Extension\\(([A-Za-z0-9_]+)\\,([12]\\.[0-7])\\) \\|\\| ([12]\\.[0-7])\\)");
                Matcher m = p.matcher(sincever);
                // m.group(1) = extension name
                // m.group(2) = extension PDF version
                // m.group(3) = core PDF version
                if (m.matches()) {
                    arl_extn_ver = Double.valueOf(m.group(2));
                    if (for_version < arl_extn_ver) {
                        // want to exclude as TSV will not exist
                        return 99;
                    }
                    else if (for_version == arl_extn_ver) {
                        // reduce expression to "fn:Extension(AAA,x.y)"
                        String s = "fn:Extension("+ m.group(1) + "," + m.group(2) + ")";
                        reduced_sincever.append(s);
                        return arl_extn_ver;
                    }
                    else {
                        // after the extension-specific version so same as TSV input
                        reduced_sincever.append(sincever);
                        return arl_extn_ver;
                    }
                }
                assert(false) : "Unexpected processing of fn:Eval(fn:Extension(AAA,x.y) || a.b)!";
            }
            else if (sincever.contains(",")) {
                // Predicate: fn:Extension(AAA,x.y) - extension AAA but only since x.y
                Pattern p = Pattern.compile("fn:Extension\\(([A-Za-z0-9_]+)\\,([12]\\.[0-7])\\)");
                Matcher m = p.matcher(sincever);
                // m.group(1) = extension name
                // m.group(2) = extension PDF version
                if (m.matches()) {
                    arl_extn_ver = Double.valueOf(m.group(2));
                    if (for_version < arl_extn_ver) {
                        // want to drop as TSV will not exist
                        return 99;
                    }
                    else if (for_version == arl_extn_ver) {
                        // reduce to just "fn:Extension(AAA)"
                        String s = "fn:Extension(" + m.group(1) + ")";
                        reduced_sincever.append(s);
                        return arl_extn_ver;
                    }
                    else { // for_version > arl_extn_ver
                        // same as input
                        reduced_sincever.append(sincever);
                        return arl_extn_ver;
                    }
                }
                assert(false) : "Unexpected processing of fn:Extension(AAA,x.y)";
            }
            else { // Predicate: fn:Extension(AAA) = keep for all versions of PDF
                reduced_sincever.append(sincever);
                return 1.0;
            }
        }
        else { // Just a normal PDF version
            reduced_sincever.append(sincever);
            return Double.parseDouble(sincever);
        }
        assert(false) : "Unexpected processing in reduceSinceVersion() method";
        return 99;
    }
    
}
