/*
 * XMLQuery.java
 * Copyright 2020 PDF Association, Inc. https://www.pdfa.org
 *
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR001119C0079.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material are those of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA). Approved for public release.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contributors: Roman Toda, Frantisek Forgac, Normex
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
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 *
 * @author fero
 */
public class TSVUpdater {

    final double[] pdf_version = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0};
    static String path_to_tsv_files = "";
    
    public TSVUpdater(){
        path_to_tsv_files = System.getProperty("user.dir") + "/tsv/latest/";
        for(double x : pdf_version){
            deleteContent(x);
            createTSV(x);
        }
    }
    
    private static void createTSV(double version){
        final String delimiter = "\t";
        
        File[] list_of_files = null;
        File folder = new File(path_to_tsv_files);
        list_of_files = folder.listFiles();
        
        for(File file : list_of_files){
            if(file.isFile() && file.canRead() && file.exists()){
                try {
                    BufferedReader tsv_reader;
                    String temp = path_to_tsv_files;
                    String file_name = file.getName().substring(0, file.getName().length()-4);
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    String[] row = null;
                    String output_string = "";
                    String entry = "";
                    System.out.println("================\nProcessing " +file_name + " for version " +version);
                    String current_line = tsv_reader.readLine();
                    if (current_line != null) {
                        output_string = current_line + "\n";
                    }
                    while ((current_line = tsv_reader.readLine()) != null) {
                        row = current_line.split(delimiter, -1);
                        String key_name = row[0];
                        System.out.println("\tkey: " +key_name);
                        /*
                        if(row.length!=12){
                            System.out.println(file_name + " "+ key_name+ " " + row.length);
                        }
                        */
                        String data_type = row[1];
                        String since_version = row[2];
                        String deprecated = row[3];
                        String required = row[4];
                        String indirect_ref = row[5];
                        String inheritable = row[6];
                        String default_value = row[7];
                        String possible_values = row[8];
                        String special_case = row[9];
                        String links = row[10];
                        String links_with_functions = "";
                        if(!links.isBlank()){
                            links_with_functions = addSinceVersionFuncToLinks(links, version);
                        }
                        String notes = row[11];
                        if(Double.parseDouble(since_version) <= version){
                            String record =
                                    key_name + delimiter +
                                    data_type + delimiter +
                                    since_version + delimiter +
                                    deprecated + delimiter +
                                    required + delimiter +
                                    indirect_ref + delimiter +
                                    inheritable + delimiter +
                                    default_value + delimiter +
                                    possible_values + delimiter +
                                    special_case + delimiter +
                                    //links + delimiter +
                                    links_with_functions + delimiter +
                                    notes;
                            entry += record + "\n";
                        }
                    }
                    if(!entry.isEmpty()){
                        output_string += entry;
                        WriteToFile(output_string, file_name, version);
                    }
                } catch (FileNotFoundException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                }
                
            }
        }
    }

    private static void WriteToFile(String output_string, String file, double version) {
        BufferedWriter out = null;
        
        String path = "";
        path +=  System.getProperty("user.dir") + "/tsv/";
        path += version + "/";
        path += file +".tsv";

        try {
            FileWriter fstream = new FileWriter(path, false);
            out = new BufferedWriter(fstream);
            out.write(output_string);
        }
        catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
        }
        finally {
            if(out != null) {
                try {
                    out.close();
                } catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }

    private void deleteContent(double x) {        
        String path = "";
        path +=  System.getProperty("user.dir") + "/tsv/";
        path += x + "/";
        
        File[] list_of_files = null;
        File folder = new File(path);
        list_of_files = folder.listFiles();
        
        for(File file : list_of_files){
            file.delete();
        }
    }
    
    private static String addSinceVersionFuncToLinks(String links, double version){
        String result = "";
        final String function = "fn:SinceVersion";
        
        String[] links2 = links.split(";"); // split by data types
        for(int i = 0; i < links2.length; i++){
            String _links = "";
            links2[i] = links2[i].replace("[", "");
            links2[i] = links2[i].replace("]", "");
            String temp = commaSplit(links2[i]);
            String[] links3 = temp.split(",,"); // split by data values
            for(int j = 0; j < links3.length; j++){
                if(!links3[j].isBlank()){
                    //double object_intro = getObjectVersion(links3[j]);
                    //System.out.println("\t\tObject:" + links3[j] + " was added in " + object_intro);
                    // remove if statement to create functions everywhere
                    String mydata = links3[j];
                    double ver = getVersion(mydata);

                    String obj = getLink(mydata);
                    
                    if(ver>version){
                        // comment out line below to create links without functions
                        //_links += function + "(" +  object_intro + "," + links3[j] +")";
                    }else{
                        _links += obj;
                    }
                    if(j+1 != links3.length){
                        _links += ",";
                    }
                }
            }       
            // removes all commas at the beginning of the string,
            // commas behind commas,
            // trailing commas
            _links = _links.replaceAll("^,*|(?<=,),|,*$", "");
               
            if(i+1 != links2.length){
                result += "["+ _links +"];";
            }else{
                result += "["+ _links +"]";
            }
        }
        System.out.println("LINK RESULT: " +result +"\n------------");
        return result;
    }
    
    private static double getObjectVersion(String link){
        double result = 10.;
        
        String object = link + ".tsv";
        final String delimiter = "\t";
        
        File[] list_of_files = null;
        File folder = new File(path_to_tsv_files);
        list_of_files = folder.listFiles();
        
        for(File file : list_of_files){
            if(file.getName().equals(object) && file.isFile() && file.canRead() && file.exists()){
                //find the lowest since version value
                try {
                    BufferedReader tsv_reader;
                    String temp = path_to_tsv_files;
                    String file_name = file.getName().substring(0, file.getName().length()-4);
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    String[] row = null;
                    
                    String current_line = tsv_reader.readLine();
                    if (current_line != null) {
                        String unused = current_line + "\n";
                    }
                    while ((current_line = tsv_reader.readLine()) != null) {
                        row = current_line.split(delimiter, -1);
                        double current_value = Double.parseDouble(row[2]);
                        if(current_value < result){
                            result = current_value;
                        }
                    }
                } catch (FileNotFoundException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
        return result;
    }
    private static String commaSplit(String s) {
        String result = "";
        int counter = 0;
        for(char ch: s.toCharArray()){
            if(ch == '('){
                counter++;
                result += "(";
            }else if (ch == ')'){
                counter--;
                result += ")";
            }else if (ch == ',' && counter == 0){
                result += ",,";
            }else{
                result += ch;
            }
        }
        return result;
    } 
    private static double getVersion(String mydata) {
        double ver = 1.0;
        Pattern pattern = Pattern.compile("\\((.*?),");
        Matcher matcher = pattern.matcher(mydata);
        if (matcher.find())
        {
            System.out.println(matcher.group(1));
            ver = Double.parseDouble(matcher.group(1));
        }
        return ver;
    }
    private static String getLink(String mydata) {
        String link = "";
        Pattern pattern = Pattern.compile(",(.*?)\\)");
        Matcher matcher = pattern.matcher(mydata);
        if (matcher.find())
        {
            System.out.println(matcher.group(1));
            link =matcher.group(1);
        }
        return link;
    }
}
